#include <plasma_dsp/filter.h>
#include <plasma_dsp/linear_fm_waveform.h>
// #include <qwt/qwt_plot.h>
// #include <qwt/qwt_plot_curve.h>
// #include <stdlib.h>

// #include <QApplication>
#include <nlohmann/json.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/thread.hpp>
#include <filesystem>
#include <iostream>


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

int main(int argc, char* argv[])
{
    // TODO: These should be program option parameters
    double mcr = 184.32e6;
    std::vector<double> samp_rates(10);
    for (size_t i = 0; i < samp_rates.size(); i++) {
        samp_rates[i] = mcr / (i + 1);
    }

    // std::vector<double> samp_rates = { 10e6, 20e6, 30e6, 40e6, 50e6 };
    std::string args = "master_clock_rate=" + std::to_string(mcr);
    double freq = 5e9;
    // Initialzie the USRP object
    uhd::usrp::multi_usrp::sptr usrp;

    double start_time = 0.2;
    boost::thread_group tx_thread;
    std::vector<double> master_clock_rates(samp_rates.size());
    std::vector<size_t> delays(samp_rates.size());
    std::string radio_type;
    for (size_t i = 0; i < samp_rates.size(); i++) {
        // Set up the USRP device
        usrp = uhd::usrp::multi_usrp::make(args);
        usrp->set_tx_freq(freq);
        usrp->set_rx_freq(freq);
        // Set the gain to half of the max gain on Tx and Rx. This should be
        // plenty of gain to get a clean matched filter peak in loopback
        double tx_gain = usrp->get_tx_gain_range().stop() * 0.5;
        double rx_gain = usrp->get_tx_gain_range().stop() * 0.5;
        usrp->set_tx_gain(tx_gain);
        usrp->set_rx_gain(rx_gain);
        usrp->set_tx_rate(samp_rates[i]);
        usrp->set_rx_rate(samp_rates[i]);

        master_clock_rates[i] = usrp->get_master_clock_rate();
        if (i == 0) {
            radio_type = usrp->get_mboard_name();
        }
        // Initialize the waveform object
        double bandwidth = samp_rates[i] / 2;
        double pulse_width = 20e-6;
        double prf = 10e3;
        plasma::LinearFMWaveform waveform(bandwidth, pulse_width, prf, samp_rates[i]);
        Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();

        // Set up Rx buffer
        size_t num_samp_rx = waveform_data.size();
        std::vector<std::complex<float>*> rx_buff_ptrs;
        std::vector<std::complex<float>> rx_buff(num_samp_rx, 0);
        rx_buff_ptrs.push_back(&rx_buff.front());

        // Set up Tx buffer
        std::vector<std::complex<float>>* tx_buff = new std::vector<std::complex<float>>(
            waveform_data.data(), waveform_data.data() + waveform_data.size());
        std::vector<std::complex<float>*> tx_buff_ptrs;
        tx_buff_ptrs.push_back(&tx_buff->front());

        // Send the  data to be used for calibration
        uhd::time_spec_t time_now = usrp->get_time_now();
        tx_thread.create_thread(boost::bind(&transmit,
                                            usrp,
                                            tx_buff_ptrs,
                                            1,
                                            waveform_data.size(),
                                            time_now + start_time));
        receive(usrp, rx_buff_ptrs, num_samp_rx, time_now + start_time);
        tx_thread.join_all();

        // Create the matched filter
        Eigen::ArrayXXcf x(rx_buff.size(), 1);
        x.col(0) = Eigen::Map<Eigen::ArrayXcf>(rx_buff.data(), rx_buff.size());
        Eigen::ArrayXcf h = waveform.MatchedFilter().cast<std::complex<float>>();

        // Do convolution and compute the delay of the matched filter peak
        Eigen::ArrayXXcf y = plasma::conv(x, h);
        size_t argmax = 0;
        for (int i = 0; i < y.size(); i++) {
            argmax = (abs(y(i, 0)) > abs(y(argmax, 0))) ? i : argmax;
        }
        delays[i] = argmax - h.size();
        // Output the results for the current configuration
        UHD_LOG_INFO("PLASMA",
                     "At MCR = " << master_clock_rates[i] / 1e6
                                 << " MHz, sample rate = " << samp_rates[i] / 1e6
                                 << " MHz, delay is " << delays[i] << " samples");


        // Reset the device for the next configuration
        usrp.reset();
    }
    // Save the calibration results to a json file
    const std::string homedir = getenv("HOME");
    // Create uhd hidden folder if it doesn't already exist
    if (not std::filesystem::exists(homedir + "/.uhd")) {
        std::filesystem::create_directory(homedir + "/.uhd");
    }
    // If data already exists in the file, keep it
    nlohmann::json json;
    std::ifstream prev_data(homedir + "/.uhd/delay_calibration.json");
    if (prev_data)
        prev_data >> json;
    prev_data.close();
    // Write the data to delay_calibration.json
    std::ofstream outfile(homedir + "/.uhd/delay_calibration.json");
    for (size_t i = 0; i < samp_rates.size(); i++) {
        // Check if radio/clock rate/sample rate triplet already exists
        bool exists = false;
        for (auto& config : json[radio_type]) {
            if (config["samp_rate"] == samp_rates[i] and
                config["master_clock_rate"] == master_clock_rates[i]) {
                config["delay"] = delays[i];
                exists = true;
                break;
            }
        }
        // Calibration configuration not found, add it
        if (not exists)
            json[radio_type].push_back({ { "master_clock_rate", master_clock_rates[i] },
                                         { "samp_rate", samp_rates[i] },
                                         { "delay", delays[i] } });
    }
    outfile << json.dump(true);
    outfile.close();

    return EXIT_SUCCESS;
}