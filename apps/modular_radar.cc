//
// Copyright 2011-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <atomic>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
using namespace std::chrono_literals;

namespace {
constexpr auto CLOCK_TIMEOUT = 1000ms; // 1000mS timeout for external clock locking
} // namespace

using start_time_type = std::chrono::time_point<std::chrono::steady_clock>;

/***********************************************************************
 * Test result variables
 **********************************************************************/
std::atomic_ullong num_overruns{ 0 };
std::atomic_ullong num_underruns{ 0 };
std::atomic_ullong num_rx_samps{ 0 };
std::atomic_ullong num_tx_samps{ 0 };
std::atomic_ullong num_dropped_samps{ 0 };
std::atomic_ullong num_seq_errors{ 0 };
std::atomic_ullong num_seqrx_errors{ 0 }; // "D"s
std::atomic_ullong num_late_commands{ 0 };
std::atomic_ullong num_timeouts_rx{ 0 };
std::atomic_ullong num_timeouts_tx{ 0 };

/***********************************************************************
 * Benchmark RX Rate
 **********************************************************************/
void receive(uhd::usrp::multi_usrp::sptr usrp,
             const std::string& rx_cpu,
             uhd::rx_streamer::sptr rx_stream,
             size_t spb,
             std::atomic<bool>& burst_timer_elapsed,
             bool elevate_priority,
             double adjusted_rx_delay,
             double user_rx_delay,
             bool rx_stream_now)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // setup variables and allocate buffer
    uhd::rx_metadata_t md;
    if (spb == 0) {
        spb = rx_stream->get_max_num_samps();
    }
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(rx_cpu));
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    bool had_an_overflow = false;
    uhd::time_spec_t last_time;
    const double rate = usrp->get_rx_rate();

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = spb;

    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(adjusted_rx_delay);
    cmd.stream_now = rx_stream_now;
    rx_stream->issue_stream_cmd(cmd);

    const float burst_pkt_time = std::max<float>(0.100f, (2 * spb / rate));
    float recv_timeout = burst_pkt_time + (adjusted_rx_delay);

    bool stop_called = false;
    while (true) {
        if (burst_timer_elapsed and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        try {
            num_rx_samps += rx_stream->recv(buffs, cmd.num_samps, md, recv_timeout) *
                            rx_stream->get_num_channels();
            recv_timeout = burst_pkt_time;
        } catch (uhd::io_error& e) {
            std::cerr << "Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        // handle the error codes
        switch (md.error_code) {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if (had_an_overflow) {
                had_an_overflow = false;
                const long dropped_samps = (md.time_spec - last_time).to_ticks(rate);
                if (dropped_samps < 0) {
                    std::cerr << "Timestamp after overrun recovery "
                                 "ahead of error timestamp! Unable to calculate "
                                 "number of dropped samples."
                                 "(Delta: "
                              << dropped_samps << " ticks)\n";
                }
                num_dropped_samps += std::max<long>(1, dropped_samps);
            }
            if ((burst_timer_elapsed or stop_called) and md.end_of_burst) {
                return;
            }
            break;

        // ERROR_CODE_OVERFLOW can indicate overflow or sequence error
        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            last_time = md.time_spec;
            had_an_overflow = true;
            // check out_of_sequence flag to see if it was a sequence error or
            // overflow
            if (!md.out_of_sequence) {
                num_overruns++;
            } else {
                num_seqrx_errors++;
                std::cerr << "[Detected Rx sequence error." << std::endl;
            }
            break;

        case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
            std::cerr << "Receiver error: " << md.strerror() << ", restart streaming..."
                      << std::endl;
            num_late_commands++;
            // Radio core will be in the idle state. Issue stream command to restart
            // streaming.
            cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.05);
            cmd.stream_now = (buffs.size() == 1);
            rx_stream->issue_stream_cmd(cmd);
            break;

        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
            if (burst_timer_elapsed) {
                return;
            }
            std::cerr << "Receiver error: " << md.strerror() << ", continuing..."
                      << std::endl;
            num_timeouts_rx++;
            break;

            // Otherwise, it's an error
        default:
            std::cerr << "Receiver error: " << md.strerror() << std::endl;
            std::cerr << "Unexpected error on recv, continuing..." << std::endl;
            break;
        }
    }
}

/***********************************************************************
 * Benchmark TX Rate
 **********************************************************************/
void transmit(uhd::usrp::multi_usrp::sptr usrp,
              const std::string& tx_cpu,
              uhd::tx_streamer::sptr tx_stream,
              std::atomic<bool>& burst_timer_elapsed,
              const size_t spb,
              bool elevate_priority,
              double tx_delay)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // setup variables and allocate buffer
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(tx_cpu));
    std::vector<const void*> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(tx_delay);

    double timeout = 0.1 + tx_delay;

    while (not burst_timer_elapsed) {
        const size_t num_tx_samps_sent_now =
            tx_stream->send(buffs, spb, md, timeout) * tx_stream->get_num_channels();
        num_tx_samps += num_tx_samps_sent_now;
        md.has_time_spec = false;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

void transmit_async_helper(uhd::tx_streamer::sptr tx_stream,
                           std::atomic<bool>& burst_timer_elapsed)
{
    // setup variables and allocate buffer
    uhd::async_metadata_t async_md;
    bool exit_flag = false;

    while (true) {
        if (burst_timer_elapsed) {
            exit_flag = true;
        }

        if (not tx_stream->recv_async_msg(async_md)) {
            if (exit_flag == true)
                return;
            continue;
        }

        // handle the error codes
        switch (async_md.event_code) {
        case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
            return;

        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
            num_underruns++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
        case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
            num_seq_errors++;
            break;

        default:
            std::cerr << "Event code: " << async_md.event_code << std::endl;
            std::cerr << "Unexpected event on async recv, continuing..." << std::endl;
            break;
        }
    }
}

/***********************************************************************
 * Main code + dispatcher
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    std::string rx_subdev, tx_subdev;
    std::string rx_stream_args, tx_stream_args;
    double duration;
    double rx_rate, tx_rate;
    std::string rx_otw, tx_otw;
    std::string rx_cpu, tx_cpu;
    std::string ref, pps;
    std::string channel_list, rx_channel_list, tx_channel_list;
    std::atomic<bool> burst_timer_elapsed(false);
    double tx_delay, rx_delay, adjusted_tx_delay, adjusted_rx_delay;
    bool rx_stream_now = false;
    std::string priority;
    bool elevate_priority = false;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for the test in seconds")
        ("rx_subdev", po::value<std::string>(&rx_subdev), "specify the device subdev for RX")
        ("tx_subdev", po::value<std::string>(&tx_subdev), "specify the device subdev for TX")
        ("rx_stream_args", po::value<std::string>(&rx_stream_args)->default_value(""), "stream args for RX streamer")
        ("tx_stream_args", po::value<std::string>(&tx_stream_args)->default_value(""), "stream args for TX streamer")
        ("rx_rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx_rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx_channels", po::value<std::string>(&rx_channel_list), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx_channels", po::value<std::string>(&tx_channel_list), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        // NOTE: tx_delay defaults to 0.25 while rx_delay defaults to 0.05 when left unspecified
        // in multi-channel and multi-streamer configurations.
        ("tx_delay", po::value<double>(&tx_delay)->default_value(0.0), "delay before starting TX in seconds")
        ("rx_delay", po::value<double>(&rx_delay)->default_value(0.0), "delay before starting RX in seconds")
        ("priority", po::value<std::string>(&priority)->default_value("normal"), "thread priority (normal, high)")
        ("multi_streamer", "Create a separate streamer per channel")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help") or (vm.count("rx_rate") + vm.count("tx_rate")) == 0) {
        std::cout << boost::format("UHD Benchmark Rate %s") % desc << std::endl;
        std::cout << "    Specify --rx_rate for a receive-only test.\n"
                     "    Specify --tx_rate for a transmit-only test.\n"
                     "    Specify both options for a full-duplex test.\n"
                  << std::endl;
        return ~0;
    }

    if (priority == "high") {
        uhd::set_thread_priority_safe();
        elevate_priority = true;
    }

    adjusted_tx_delay = tx_delay;
    adjusted_rx_delay = rx_delay;

    // create a usrp device
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("rx_subdev")) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    if (vm.count("tx_subdev")) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    boost::thread_group thread_group;

    std::vector<size_t> rx_channel_nums(1, 0);
    std::vector<size_t> tx_channel_nums(1, 0);

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(0.0);

    // spawn the receive test thread
    if (vm.count("rx_rate")) {
        usrp->set_rx_rate(rx_rate);
        if ((rx_delay == 0.0) && rx_channel_nums.size() > 1) {
            adjusted_rx_delay = std::max(rx_delay, 0.05);
        }
        if (rx_delay == 0.0 && (rx_channel_nums.size() == 1)) {
            rx_stream_now = true;
        }

        size_t spb = 0;
        // create a receive streamer
        uhd::stream_args_t stream_args(rx_cpu, rx_otw);
        stream_args.channels = rx_channel_nums;
        stream_args.args = uhd::device_addr_t(rx_stream_args);
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
        auto rx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
            receive(usrp,
                    rx_cpu,
                    rx_stream,
                    spb,
                    burst_timer_elapsed,
                    elevate_priority,
                    adjusted_rx_delay,
                    rx_delay,
                    rx_stream_now);
        });
        uhd::set_thread_name(rx_thread, "bmark_rx_stream");
    }

    // spawn the transmit test thread
    if (vm.count("tx_rate")) {
        usrp->set_tx_rate(tx_rate);
        if (tx_delay == 0.0 && tx_channel_nums.size() > 1) {
            adjusted_tx_delay = std::max(tx_delay, 0.25);
        }

        // create a transmit streamer
        uhd::stream_args_t stream_args(tx_cpu, tx_otw);
        stream_args.channels = tx_channel_nums;
        stream_args.args = uhd::device_addr_t(tx_stream_args);
        uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
        const size_t max_spp = tx_stream->get_max_num_samps();
        size_t spp = max_spp;
        size_t spb = spp;
        std::cout << boost::format("Setting TX spp to %u\n") % spp;
        auto tx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
            transmit(usrp,
                     tx_cpu,
                     tx_stream,
                     burst_timer_elapsed,
                     spb,
                     elevate_priority,
                     adjusted_tx_delay);
        });
        uhd::set_thread_name(tx_thread, "bmark_tx_stream");
        auto tx_async_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
            transmit_async_helper(tx_stream, burst_timer_elapsed);
        });
        uhd::set_thread_name(tx_async_thread, "bmark_tx_helper");
    }

    // Sleep for the required duration (add any initial delay).
    // If you are benchmarking Rx and Tx at the same time, Rx threads will run longer
    // than specified duration if tx_delay > rx_delay because of the overly simplified
    // logic below and vice versa.
    if (vm.count("rx_rate") and vm.count("tx_rate")) {
        duration += std::max(adjusted_rx_delay, adjusted_tx_delay);
    } else if (vm.count("rx_rate")) {
        duration += adjusted_rx_delay;
    } else {
        duration += adjusted_tx_delay;
    }
    const int64_t secs = int64_t(duration);
    const int64_t usecs = int64_t((duration - secs) * 1e6);
    std::this_thread::sleep_for(std::chrono::seconds(secs) +
                                std::chrono::microseconds(usecs));

    // interrupt and join the threads
    burst_timer_elapsed = true;
    thread_group.join_all();

    std::cout << "Benchmark complete." << std::endl << std::endl;

    // print summary
    std::cout << std::endl
              << boost::format("Benchmark rate summary:\n"
                               "  Num received samples:     %u\n"
                               "  Num dropped samples:      %u\n"
                               "  Num overruns detected:    %u\n"
                               "  Num transmitted samples:  %u\n"
                               "  Num sequence errors (Tx): %u\n"
                               "  Num sequence errors (Rx): %u\n"
                               "  Num underruns detected:   %u\n"
                               "  Num late commands:        %u\n"
                               "  Num timeouts (Tx):        %u\n"
                               "  Num timeouts (Rx):        %u\n") %
                     num_rx_samps % num_dropped_samps % num_overruns % num_tx_samps %
                     num_seq_errors % num_seqrx_errors % num_underruns %
                     num_late_commands % num_timeouts_tx % num_timeouts_rx
              << std::endl;
    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}