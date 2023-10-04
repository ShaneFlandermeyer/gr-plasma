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
std::atomic_ullong num_rx_samps{ 0 };
std::atomic_ullong num_tx_samps{ 0 };

/***********************************************************************
 * USRP configuration functions
 **********************************************************************/
void config_usrp(uhd::usrp::multi_usrp::sptr& usrp,
                 const std::string& args,
                 const double tx_rate,
                 const double rx_rate,
                 const double tx_freq,
                 const double rx_freq,
                 const std::string& tx_subdev = "",
                 const std::string& rx_subdev = "",
                 bool verbose = false)
{
    usrp = uhd::usrp::multi_usrp::make(args);
    if (not tx_subdev.empty()) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }
    if (not rx_subdev.empty()) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    usrp->set_tx_rate(tx_rate);
    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_freq(tx_freq);
    usrp->set_rx_freq(rx_freq);

    if (verbose) {
        std::cout << boost::format("Using Device: %s") % usrp->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp->get_rx_freq() / 1e6)
                  << std::endl;
    }
}

/***********************************************************************
 * Transmit and receive functions
 **********************************************************************/
void receive(uhd::usrp::multi_usrp::sptr usrp,
             uhd::rx_streamer::sptr rx_stream,
             std::vector<void*> buffs,
             size_t buff_size,
             std::atomic<bool>& finished,
             bool elevate_priority,
             double adjusted_rx_delay,
             bool rx_stream_now)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // setup variables and allocate buffer
    uhd::rx_metadata_t md;

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = buff_size;

    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(adjusted_rx_delay);
    cmd.stream_now = rx_stream_now;
    rx_stream->issue_stream_cmd(cmd);

    float recv_timeout = 0.1 + adjusted_rx_delay;

    bool stop_called = false;
    while (true) {
        if (finished and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        try {
            num_rx_samps += rx_stream->recv(buffs, cmd.num_samps, md, recv_timeout) *
                            rx_stream->get_num_channels();
            recv_timeout = 0.1;
        } catch (uhd::io_error& e) {
            std::cerr << "Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        // Handle errors
        switch (md.error_code) {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if ((finished or stop_called) and md.end_of_burst) {
                return;
            }
            break;
        default:
            break;
        }
    }
}

void transmit(uhd::usrp::multi_usrp::sptr usrp,
              uhd::tx_streamer::sptr tx_stream,
              std::vector<void*> buffs,
              size_t buff_size,
              std::atomic<bool>& finished,
              bool elevate_priority,
              double tx_delay)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(tx_delay);

    double timeout = 0.1 + tx_delay;
    while (not finished) {
        const size_t n_sent = tx_stream->send(buffs, buff_size, md, timeout) *
                              tx_stream->get_num_channels();
        num_tx_samps += n_sent;
        md.has_time_spec = false;
        timeout = 0.1;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
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
    std::atomic<bool> finished(false);
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
    bool verbose = true;
    uhd::usrp::multi_usrp::sptr usrp;
    config_usrp(
        usrp, args, tx_rate, rx_rate, 2.4e9, 2.4e9, tx_subdev, rx_subdev, verbose);

    usrp->set_time_now(0.0);
    std::vector<size_t> rx_channel_nums(1, 0);
    std::vector<size_t> tx_channel_nums(1, 0);

    boost::thread_group thread_group;


    // spawn the receive test thread
    std::vector<char> rx_buff, tx_buff;
    std::vector<void*> rx_buffs, tx_buffs;
    if (vm.count("rx_rate")) {
        usrp->set_rx_rate(rx_rate);
        if (rx_delay == 0.0 && rx_channel_nums.size() > 1) {
            adjusted_rx_delay = std::max(rx_delay, 0.05);
        }
        if (rx_delay == 0.0 && (rx_channel_nums.size() == 1)) {
            rx_stream_now = true;
        }

        // create a receive streamer
        uhd::stream_args_t stream_args(rx_cpu, rx_otw);
        stream_args.channels = rx_channel_nums;
        stream_args.args = uhd::device_addr_t(rx_stream_args);
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
        size_t buff_size = rx_stream->get_max_num_samps();
        rx_buff = std::vector<char>(buff_size * uhd::convert::get_bytes_per_item(rx_cpu));
        for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
            rx_buffs.push_back(&rx_buff.front());
        auto rx_thread = thread_group.create_thread([=, &finished]() {
            receive(usrp,
                    rx_stream,
                    rx_buffs,
                    buff_size,
                    finished,
                    elevate_priority,
                    adjusted_rx_delay,
                    rx_stream_now);
        });
        uhd::set_thread_name(rx_thread, "rx_stream");
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
        size_t buff_size = tx_stream->get_max_num_samps();
        std::cout << boost::format("Setting TX buff_size to %u\n") % buff_size;
        tx_buff = std::vector<char>(buff_size * uhd::convert::get_bytes_per_item(tx_cpu));
        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
            tx_buffs.push_back(&tx_buff.front()); // same buffer for each channel
        auto tx_thread = thread_group.create_thread([=, &finished]() {
            transmit(usrp,
                     tx_stream,
                     tx_buffs,
                     buff_size,
                     finished,
                     elevate_priority,
                     adjusted_tx_delay);
        });
        uhd::set_thread_name(tx_thread, "tx_stream");
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
    finished = true;
    thread_group.join_all();

    std::cout << "Benchmark complete." << std::endl << std::endl;

    // print summary
    std::cout << std::endl
              << boost::format("Benchmark rate summary:\n"
                               "  Num received samples:     %u\n"
                               "  Num transmitted samples:  %u\n") %
                     num_rx_samps % num_tx_samps
              << std::endl;
    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}