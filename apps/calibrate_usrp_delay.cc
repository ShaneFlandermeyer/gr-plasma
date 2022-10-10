#include <nlohmann/json.hpp>
#include <plasma_dsp/linear_fm_waveform.h>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float>*> buff_ptrs,
              size_t num_pulses,
              size_t num_samps_pulse,
              uhd::time_spec_t start_time)
{
    static bool first = true;

    // static bool first = true;
    static uhd::stream_args_t tx_stream_args("fc32", "sc16");
    uhd::tx_streamer::sptr tx_stream;
    // tx_stream.reset();
    if (first) {
        first = false;
        tx_stream_args.channels.push_back(0);
    }
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    // Create metadata structure
    static uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst = false;
    tx_md.has_time_spec = (start_time.get_real_secs() > 0 ? true : false);
    tx_md.time_spec = start_time;

    for (size_t ipulse = 0; ipulse < num_pulses; ipulse++) {
        tx_stream->send(buff_ptrs, num_samps_pulse, tx_md, 0.5);
        tx_md.start_of_burst = false;
        tx_md.has_time_spec = false;
    }

    // Send mini EOB packet
    tx_md.has_time_spec = false;
    tx_md.end_of_burst = true;
    tx_stream->send("", 0, tx_md);
}

long int receive(uhd::usrp::multi_usrp::sptr usrp,
                 std::vector<std::complex<float>*> buff_ptrs,
                 size_t num_samps,
                 uhd::time_spec_t start_time)
{
    static bool first = true;

    static size_t channels = buff_ptrs.size();
    static std::vector<size_t> channel_vec;
    uhd::stream_args_t stream_args("fc32", "sc16");
    uhd::rx_streamer::sptr rx_stream;
    if (first) {
        first = false;
        if (channel_vec.size() == 0) {
            for (size_t i = 0; i < channels; i++) {
                channel_vec.push_back(i);
            }
        }
        stream_args.channels = channel_vec;
    }
    rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;

    // Set up streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = num_samps;
    stream_cmd.time_spec = start_time;
    stream_cmd.stream_now = (start_time.get_real_secs() > 0 ? false : true);
    rx_stream->issue_stream_cmd(stream_cmd);

    size_t max_num_samps = rx_stream->get_max_num_samps();
    size_t num_samps_total = 0;
    std::vector<std::complex<float>*> buff_ptrs2(buff_ptrs.size());
    double timeout = 0.5 + start_time.get_real_secs();
    while (num_samps_total < num_samps) {
        // Move storing pointer to correct location
        for (size_t i = 0; i < channels; i++)
            buff_ptrs2[i] = &(buff_ptrs[i][num_samps_total]);

        // Sampling data
        size_t samps_to_recv = std::min(num_samps - num_samps_total, max_num_samps);
        size_t num_rx_samps = rx_stream->recv(buff_ptrs2, samps_to_recv, md, timeout);
        timeout = 0.5;

        num_samps_total += num_rx_samps;

        // handle the error code
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
            break;
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)
            break;
    }

    if (num_samps_total < num_samps)
        return -((long int)num_samps_total);
    else
        return (long int)num_samps_total;
}

inline void handle_receive_errors(const uhd::rx_metadata_t& rx_meta)
{
    if (rx_meta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
        std::cout << boost::format("No packet received, implementation timed-out.")
                  << std::endl;
        return;
    }
    if (rx_meta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        throw std::runtime_error(
            str(boost::format("Receiver error %s") % rx_meta.strerror()));
    }
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    af::info();
    // Save the calibration results to a json file
    const std::string homedir = getenv("HOME");

    // Initialzie the USRP object
    uhd::usrp::multi_usrp::sptr usrp;

    // TODO: These should be program option parameters
    std::vector<double> rates;
    std::string args;
    std::string filename;
    double tx_gain, rx_gain;
    double freq;
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()("help", "help message")
    ("rates", po::value<std::vector<double>>()->multitoken(), "List of samples rates")
    ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
    ("tx_gain", po::value<double>(&tx_gain), "Transmit gain")
    ("rx_gain", po::value<double>(&rx_gain), "Receive gain")
    ("freq", po::value<double>(&freq)->default_value(5e9), "Center Frequency")
    ("filename", po::value<std::string>(&filename)->default_value(homedir + "/.uhd/delay_calibration.json"), "Output json file")
    ;
    usrp = uhd::usrp::multi_usrp::make(args);
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("gr-plasma Calibrate USRP Delay %s") % desc
                  << std::endl;
        std::cout << "The DSP in USRP devices introduces a delay that is constant for a "
                     "given sample rate and master clock rate. In radar processing, this "
                     "delay shows up as a constant range offset and must be corrected to "
                     "produce accurate range values. This script estimates the delay for "
                     "each sample rate/master clock rate pair in the input arguments, "
                     "and saves them to a json file for use in later processing."
                  << std::endl;
        return ~0;
    }

    if (not vm["rates"].empty()) {
        rates = vm["rates"].as<std::vector<double>>();
    } else {
        UHD_LOG_ERROR(
            "CALIBRATE_DELAY",
            "You must specify a list of sample rates to calibrate with the --rates flag");
        return EXIT_FAILURE;
    }

    if (not vm.count("tx_gain")) {
        UHD_LOG_ERROR("CALIBRATE_DELAY",
                      "You must specify a transmit gain with the --tx_gain flag");
        return EXIT_FAILURE;
    }

    if (not vm.count("rx_gain")) {
        UHD_LOG_ERROR("CALIBRATE_DELAY",
                      "You must specify a receive gain with the --rx_gain flag");
        return EXIT_FAILURE;
    }


    // Get the directory of the output file
    std::string directory;
    const size_t last_slash_idx = filename.rfind('/');
    if (std::string::npos != last_slash_idx) {
        directory = filename.substr(0, last_slash_idx);
    }
    double start_time = 0.2;
    boost::thread_group tx_thread;
    std::vector<double> master_clock_rates(rates.size());
    std::vector<size_t> delays(rates.size());
    std::string radio_type;
    for (size_t i = 0; i < rates.size(); i++) {
        // Set up the USRP device
        usrp = uhd::usrp::multi_usrp::make(args);
        usrp->set_tx_freq(freq);
        usrp->set_rx_freq(freq);
        // Set the gain to half of the max gain on Tx and Rx. This should be
        // plenty of gain to get a clean matched filter peak in loopback
        usrp->set_tx_gain(tx_gain);
        usrp->set_rx_gain(rx_gain);
        usrp->set_tx_rate(rates[i]);
        usrp->set_rx_rate(rates[i]);

        master_clock_rates[i] = usrp->get_master_clock_rate();
        if (i == 0) {
            radio_type = usrp->get_mboard_name();
        }
        // Initialize the waveform object
        double bandwidth = rates[i] / 2;
        double pulse_width = 20e-6;
        double prf = 10e3;
        plasma::LinearFMWaveform waveform(bandwidth, pulse_width, rates[i], prf);
        af::array waveform_array = waveform.step().as(c32);
        std::unique_ptr<std::complex<float>> waveform_data(
            reinterpret_cast<std::complex<float>*>(waveform_array.host<af::cfloat>()));

        // Set up Rx buffer
        size_t num_samp_rx = waveform_array.elements();
        std::vector<std::complex<float>*> rx_buff_ptrs;
        std::vector<std::complex<float>> rx_buff(num_samp_rx, 0);
        rx_buff_ptrs.push_back(&rx_buff.front());

        // Set up Tx buffer
        std::vector<std::complex<float>*> tx_buff_ptrs;
        tx_buff_ptrs.push_back(waveform_data.get());

        // Send the  data to be used for calibration
        uhd::time_spec_t time_now = usrp->get_time_now();
        tx_thread.create_thread(boost::bind(&transmit,
                                            usrp,
                                            tx_buff_ptrs,
                                            1,
                                            waveform_array.elements(),
                                            time_now + start_time));
        receive(usrp, rx_buff_ptrs, num_samp_rx, time_now + start_time);
        tx_thread.join_all();

        // Create the matched filter
        af::array x(rx_buff.size(), c32);
        x.write(reinterpret_cast<af::cfloat*>(rx_buff.data()),
                rx_buff.size() * sizeof(af::cfloat));
        af::array h = waveform.MatchedFilter();
        af::array y = af::convolve1(x, h, AF_CONV_EXPAND, AF_CONV_AUTO);
        double max;
        unsigned int argmax;
        af::max<double>(&max, &argmax, y);
        delays[i] = argmax - h.elements();
        // Output the results for the current configuration
        UHD_LOG_INFO("CALIBRATE_DELAY",
                     "At MCR = " << master_clock_rates[i] / 1e6
                                 << " MHz, sample rate = " << rates[i] / 1e6
                                 << " MHz, delay is " << delays[i] << " samples");


        // Reset the device for the next configuration
        usrp.reset();
    }
    // Save the calibration results to a json file
    // const std::string homedir = getenv("HOME");
    // Create uhd hidden folder if it doesn't already exist
    if (not std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
    }
    // If data already exists in the file, keep it
    nlohmann::json json;
    std::ifstream prev_data(filename);
    if (prev_data)
        prev_data >> json;
    prev_data.close();
    // Write the data to delay_calibration.json
    std::ofstream outfile(filename);
    for (size_t i = 0; i < rates.size(); i++) {
        // Check if radio/clock rate/sample rate triplet already exists
        bool exists = false;
        for (auto& config : json[radio_type]) {
            if (config["samp_rate"] == rates[i] and
                config["master_clock_rate"] == master_clock_rates[i]) {
                config["delay"] = delays[i];
                exists = true;
                break;
            }
        }
        // Calibration configuration not found, add it
        if (not exists)
            json[radio_type].push_back({ { "master_clock_rate", master_clock_rates[i] },
                                         { "samp_rate", rates[i] },
                                         { "delay", delays[i] } });
    }
    outfile << json.dump(true);
    outfile.close();

    return EXIT_SUCCESS;
}