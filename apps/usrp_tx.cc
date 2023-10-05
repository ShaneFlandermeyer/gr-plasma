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
 * Benchmark TX Rate
 **********************************************************************/
void benchmark_tx_rate(uhd::usrp::multi_usrp::sptr usrp,
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

    // print pre-test summary
    // auto time_stamp   = NOW();
    auto tx_rate = usrp->get_tx_rate();
    auto num_channels = tx_stream->get_num_channels();
    std::cout << boost::format("Testing transmit rate %f Msps on %u channels\n") %
                     (tx_rate / 1e6) % num_channels;

    // setup variables and allocate buffer
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(tx_cpu));
    std::vector<const void*> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(tx_delay);

    // Calculate timeout time
    // The timeout time cannot be reduced after the first packet as is done for
    // TX because the delay will only happen after the TX buffers in the FPGA
    // are full and that is dependent on several factors such as the device,
    // FPGA configuration, and device arguments.  The extra 100ms is to account
    // for overhead of the send() call (enough).
    const double burst_pkt_time = std::max<double>(0.1, (2.0 * spb / tx_rate));
    double timeout = burst_pkt_time + tx_delay;

    while (not burst_timer_elapsed) {
        const size_t num_tx_samps_sent_now =
            tx_stream->send(buffs, spb, md, timeout) * tx_stream->get_num_channels();
        md.has_time_spec = false;
        num_tx_samps += num_tx_samps_sent_now;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

/***********************************************************************
 * Main code + dispatcher
 **********************************************************************/
int main(int argc, char* argv[])
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
    bool random_nsamps = false;
    std::atomic<bool> burst_timer_elapsed(false);
    size_t overrun_threshold, underrun_threshold, drop_threshold, seq_threshold;
    size_t rx_spp, tx_spp, rx_spb, tx_spb;
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
        ("rx_spp", po::value<size_t>(&rx_spp), "samples/packet value for RX")
        ("tx_spp", po::value<size_t>(&tx_spp), "samples/packet value for TX")
        ("rx_spb", po::value<size_t>(&rx_spb), "samples/buffer value for RX")
        ("tx_spb", po::value<size_t>(&tx_spb), "samples/buffer value for TX")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("ref", po::value<std::string>(&ref), "clock reference (internal, external, mimo, gpsdo)")
        ("pps", po::value<std::string>(&pps), "PPS source (internal, external, mimo, gpsdo)")
        ("random", "Run with random values of samples in send() and recv() to stress-test the I/O.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx_channels", po::value<std::string>(&rx_channel_list), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx_channels", po::value<std::string>(&tx_channel_list), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("overrun-threshold", po::value<size_t>(&overrun_threshold),
         "Number of overruns (O) which will declare the benchmark a failure.")
        ("underrun-threshold", po::value<size_t>(&underrun_threshold),
         "Number of underruns (U) which will declare the benchmark a failure.")
        ("drop-threshold", po::value<size_t>(&drop_threshold),
         "Number of dropped packets (D) which will declare the benchmark a failure.")
        ("seq-threshold", po::value<size_t>(&seq_threshold),
         "Number of dropped packets (D) which will declare the benchmark a failure.")
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

    // Random number of samples?
    if (vm.count("random")) {
        std::cout << "Using random number of samples in send() and recv() calls."
                  << std::endl;
        random_nsamps = true;
    }

    adjusted_tx_delay = tx_delay;
    adjusted_rx_delay = rx_delay;

    // create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1") {
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Benchmark results will be inaccurate on USRP1 due to insufficient "
                     "features.\n"
                  << std::endl;
    }
    start_time_type start_time(std::chrono::steady_clock::now());
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("rx_subdev")) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    if (vm.count("tx_subdev")) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    int num_mboards = usrp->get_num_mboards();

    boost::thread_group thread_group;

    // check that the device has sufficient RX and TX channels available
    std::vector<std::string> channel_strings;

    std::vector<size_t> tx_channel_nums;
    if (vm.count("tx_rate")) {
        if (!vm.count("tx_channels")) {
            tx_channel_list = channel_list;
        }

        boost::split(channel_strings, tx_channel_list, boost::is_any_of("\"',"));
        for (size_t ch = 0; ch < channel_strings.size(); ch++) {
            size_t chan = std::stoul(channel_strings[ch]);
            if (chan >= usrp->get_tx_num_channels()) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                tx_channel_nums.push_back(std::stoul(channel_strings[ch]));
            }
        }
    }

    std::cout << boost::format("Setting device timestamp to 0...")
              << std::endl;

    // spawn the transmit test thread
    if (vm.count("tx_rate")) {
        usrp->set_tx_rate(tx_rate);
        // Set an appropriate tx_delay value (if needed) to be used as the time_spec for
        // streaming.
        // A time_spec is needed to time align multiple channels or if the user specifies
        // a delay. Also delay start in case we are using multiple streamers to stream
        // multi channel data to avoid management transaction contention between threads
        // during setup.
        if ((tx_delay == 0.0 || vm.count("multi_streamer")) &&
            tx_channel_nums.size() > 1) {
            adjusted_tx_delay = std::max(tx_delay, 0.25);
        }

        // create a transmit streamer
        uhd::stream_args_t stream_args(tx_cpu, tx_otw);
        stream_args.channels = tx_channel_nums;
        stream_args.args = uhd::device_addr_t(tx_stream_args);
        uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
        const size_t max_spp = tx_stream->get_max_num_samps();
        size_t spp = max_spp;
        if (vm.count("tx_spp")) {
            spp = std::min(spp, tx_spp);
        }
        size_t spb = spp;
        if (vm.count("tx_spb")) {
            spb = tx_spb;
        }
        std::cout << boost::format("Setting TX spp to %u\n") % spp;
        auto tx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
            benchmark_tx_rate(usrp,
                              tx_cpu,
                              tx_stream,
                              burst_timer_elapsed,
                              spb,
                              elevate_priority,
                              adjusted_tx_delay);
        });
        uhd::set_thread_name(tx_thread, "bmark_tx_stream");
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
    const std::string threshold_err(" ERROR: Exceeds threshold!");
    const bool overrun_threshold_err =
        vm.count("overrun-threshold") and num_overruns > overrun_threshold;
    const bool underrun_threshold_err =
        vm.count("underrun-threshold") and num_underruns > underrun_threshold;
    const bool drop_threshold_err =
        vm.count("drop-threshold") and num_seqrx_errors > drop_threshold;
    const bool seq_threshold_err =
        vm.count("seq-threshold") and num_seq_errors > seq_threshold;
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

    if (overrun_threshold_err || underrun_threshold_err || drop_threshold_err ||
        seq_threshold_err) {
        std::cout << "The following error thresholds were exceeded:\n";
        if (overrun_threshold_err) {
            std::cout << boost::format("  * Overruns (%d/%d)") % num_overruns %
                             overrun_threshold
                      << std::endl;
        }
        if (underrun_threshold_err) {
            std::cout << boost::format("  * Underruns (%d/%d)") % num_underruns %
                             underrun_threshold
                      << std::endl;
        }
        if (drop_threshold_err) {
            std::cout << boost::format("  * Dropped packets (RX) (%d/%d)") %
                             num_seqrx_errors % drop_threshold
                      << std::endl;
        }
        if (seq_threshold_err) {
            std::cout << boost::format("  * Dropped packets (TX) (%d/%d)") %
                             num_seq_errors % seq_threshold
                      << std::endl;
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}