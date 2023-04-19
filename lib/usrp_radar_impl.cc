/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_impl.h"
#include <gnuradio/io_signature.h>
namespace gr {
namespace plasma {

usrp_radar::sptr usrp_radar::make(const std::string& args)
{
    return gnuradio::make_block_sptr<usrp_radar_impl>(args);
}


/*
 * The private constructor
 */
usrp_radar_impl::usrp_radar_impl(const std::string& args)
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0))
{
    d_usrp = uhd::usrp::multi_usrp::make(args);
    d_pulse_count = 0;
    d_tx_sample_count = 0;
    d_rx_sample_count = 0;

    d_meta = pmt::make_dict();


    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](const pmt::pmt_t& msg) { handle_message(msg); });
}

usrp_radar_impl::~usrp_radar_impl() {}


bool usrp_radar_impl::start()
{
    d_finished = false;
    d_main_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool usrp_radar_impl::stop()
{
    d_finished = true;
    d_main_thread.join();

    return block::stop();
}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        // Maintain any metadata that was produced by upstream blocks
        pmt::pmt_t meta = pmt::car(msg);


        double prf = 0;
        if (pmt::dict_has_key(meta, d_prf_key)) {
            prf = pmt::to_double(pmt::dict_ref(meta, d_prf_key, pmt::PMT_NIL));
        }

        d_meta = pmt::dict_update(d_meta, meta);

        // Copy Tx data into buffer
        gr::thread::scoped_lock lock(d_tx_buff_mutex);
        d_tx_buff = c32vector_elements(pmt::cdr(msg));
        if (prf > 0) {
            int n_zeros = round(d_samp_rate / prf);
            int n_zeros_end = n_zeros - d_tx_buff.size();
            std::vector<gr_complex> end_zeros(n_zeros_end, 0);
            d_tx_buff.insert(
                std::end(d_tx_buff), std::begin(end_zeros), std::end(end_zeros));
        }
        d_n_samp_pri = d_tx_buff.size();

        // The USRP will underflow on transmit if there are less than 1000 samples in the buffer. Therefore, repeat the data until there are at least 1000 samples.
        while (d_tx_buff.size() < 1000) {
            d_tx_buff.insert(std::end(d_tx_buff), std::begin(d_tx_buff), std::end(d_tx_buff));
        }
        d_armed = true;
    }
}

void usrp_radar_impl::run()
{
    // Wait for the first data to arrive
    while (not d_armed) {
        if (d_finished)
            return;
        else
            boost::this_thread::sleep(boost::posix_time::microseconds(1));
    }

    uhd::time_spec_t time_now = d_usrp->get_time_now();

    // Start the transmit thread
    auto tx_thread = gr::thread::thread(
        [this, time_now] { transmit(d_usrp, time_now + d_start_time); });
    // Start receiving in the main thread
    auto rx_thread = gr::thread::thread(
        [this, time_now] { receive(d_usrp, time_now + d_start_time); });

    // Wait for Tx and Rx to finish the current pulse
    tx_thread.join();
    rx_thread.join();
}


void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                                      uhd::time_spec_t start_time)
{
    if (d_tx_thread_priority != 0) {
        uhd::set_thread_priority_safe(d_tx_thread_priority);
    }

    // Set up stream
    uhd::stream_args_t tx_stream_args("fc32", "sc16");
    uhd::tx_streamer::sptr tx_stream;
    tx_stream_args.channels.push_back(0);
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst = false;
    tx_md.has_time_spec = true;
    tx_md.time_spec = start_time;

    while (!d_finished) {
        if (d_armed) {
            // Record when the new waveform actually started in the metadata
            d_meta =
                pmt::dict_add(d_meta, d_sample_start_key, pmt::from_long(d_tx_sample_count));
            d_armed = false;
        }

        
        tx_stream->send(d_tx_buff.data(), d_tx_buff.size(), tx_md);
        tx_md.start_of_burst = false;
        tx_md.has_time_spec = false;

        d_pulse_count++;
        d_tx_sample_count += d_tx_buff.size();
    }
    // Send a mini EOB packet
    tx_md.end_of_burst = true;
    tx_stream->send("", 0, tx_md);
}


void usrp_radar_impl::receive(uhd::usrp::multi_usrp::sptr usrp,
                              uhd::time_spec_t start_time)
{
    if (d_rx_thread_priority != 0) {
        uhd::set_thread_priority_safe(d_rx_thread_priority);
    }

    // Create receive streamer
    uhd::stream_args_t stream_args("fc32", "sc16");
    // TODO: Add multi-channel support
    stream_args.channels = std::vector<size_t>(1, 0);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Set up streaming
    uhd::rx_metadata_t rx_md;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = start_time;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Set up Rx buffers
    std::vector<gr_complex> buff(rx_stream->get_max_num_samps());
    pmt::pmt_t rx_data_pmt = pmt::make_c32vector(d_n_samp_pri, 0);
    gr_complex* rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, d_n_samp_pri);

    // Strip off the delay samples due to USRP hardware
    size_t n_samps_written = 0;
    size_t n_samps_remaining;
    size_t start, stop;
    size_t n_rx_samps;
    start = 0;
    double timeout = start_time.get_real_secs() + 0.1;

    while (!d_finished) {
        n_rx_samps = rx_stream->recv(buff.data(), buff.size(), rx_md, timeout);
        timeout = 0.1;

        // Copy as much of the rx buffer as possible to the PDU output vector. If the PDU
        // capacity is reached, send the PDU and reset the vector.
        start = d_delay_samps;
        stop = std::min(n_rx_samps, start + d_n_samp_pri - n_samps_written);
        n_samps_remaining = n_rx_samps;
        while (n_samps_remaining > 0) {
            std::copy(
                buff.begin() + start, buff.begin() + stop, rx_data_ptr + n_samps_written);
            n_samps_written += (stop - start);
            n_samps_remaining -= (stop - start);
            n_samps_remaining -= d_delay_samps;
            if (n_samps_written == d_n_samp_pri) {
                message_port_pub(d_out_port, pmt::cons(d_meta, rx_data_pmt));
                n_samps_written = 0;
            }
            start = stop;
            stop = std::min(n_rx_samps, start + d_n_samp_pri - n_samps_written);
            d_delay_samps = 0; // Only used for first receive call
        }
        d_rx_sample_count += n_rx_samps;
    }

    // Shut down the stream
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);
}

void usrp_radar_impl::read_calibration_file(const std::string& filename)
{
    std::ifstream file(filename);
    nlohmann::json json;
    d_delay_samps = 0;
    if (file) {
        file >> json;
        std::string radio_type = d_usrp->get_mboard_name();
        for (auto& config : json[radio_type]) {
            if (config["samp_rate"] == d_usrp->get_tx_rate() and
                config["master_clock_rate"] == d_usrp->get_master_clock_rate()) {
                d_delay_samps = config["delay"];
                break;
            }
        }
        if (d_delay_samps == 0)
            UHD_LOG_INFO("USRP Radar",
                         "Calibration file found, but no data exists for this "
                         "combination of radio, master clock rate, and sample rate");
    } else {
        UHD_LOG_INFO("USRP Radar", "No calibration file found");
    }

    file.close();
}

void usrp_radar_impl::set_metadata_keys(std::string center_freq_key,
                                        std::string prf_key,
                                        std::string sample_count_key)
{
    d_center_freq_key = pmt::intern(center_freq_key);
    d_prf_key = pmt::intern(prf_key);
    d_sample_start_key = pmt::intern(sample_count_key);

    d_meta = pmt::dict_add(d_meta, d_sample_start_key, pmt::from_long(d_tx_sample_count));
}

void usrp_radar_impl::set_samp_rate(const double rate)
{
    d_samp_rate = rate;
    d_usrp->set_tx_rate(d_samp_rate);
    d_usrp->set_rx_rate(d_samp_rate);
}

void usrp_radar_impl::set_tx_gain(const double gain)
{
    d_tx_gain = gain;
    d_usrp->set_tx_gain(d_tx_gain);
}

void usrp_radar_impl::set_rx_gain(const double gain)
{
    d_rx_gain = gain;
    d_usrp->set_rx_gain(d_rx_gain);
}

void usrp_radar_impl::set_tx_freq(const double freq)
{
    d_tx_freq = freq;
    d_usrp->set_tx_freq(d_tx_freq);

    // Append additional metadata to the pmt object
    d_meta = pmt::dict_add(d_meta, d_center_freq_key, pmt::from_double(d_tx_freq));
}

void usrp_radar_impl::set_rx_freq(const double freq)
{
    d_rx_freq = freq;
    d_usrp->set_rx_freq(d_rx_freq);
    while (not d_usrp->get_rx_sensor("lo_locked").to_bool()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
void usrp_radar_impl::set_start_time(const double t) { d_start_time = t; }


void usrp_radar_impl::set_tx_thread_priority(const double priority)
{
    d_tx_thread_priority = priority;
}

void usrp_radar_impl::set_rx_thread_priority(const double priority)
{
    d_rx_thread_priority = priority;
}


} /* namespace plasma */
} /* namespace gr */
