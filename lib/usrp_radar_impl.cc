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

usrp_radar::sptr usrp_radar::make(double samp_rate,
                                  double tx_gain,
                                  double rx_gain,
                                  double tx_freq,
                                  double rx_freq,
                                  double start_time,
                                  const std::string& tx_args,
                                  const std::string& rx_args)
{
    return gnuradio::make_block_sptr<usrp_radar_impl>(
        samp_rate, tx_gain, rx_gain, tx_freq, rx_freq, start_time, tx_args, rx_args);
}


/*
 * The private constructor
 */
usrp_radar_impl::usrp_radar_impl(double samp_rate,
                                 double tx_gain,
                                 double rx_gain,
                                 double tx_freq,
                                 double rx_freq,
                                 double start_time,
                                 const std::string& tx_args,
                                 const std::string& rx_args)
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_samp_rate(samp_rate),
      d_tx_gain(tx_gain),
      d_rx_gain(rx_gain),
      d_tx_freq(tx_freq),
      d_rx_freq(rx_freq),
      d_start_time(start_time),
      d_tx_args(tx_args),
      d_rx_args(rx_args)
{
    message_port_register_in(PMT_IN);
    message_port_register_out(PMT_OUT);
    set_msg_handler(PMT_IN,
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    // Initialize the USRP device
    // d_usrp.reset();
    d_usrp = uhd::usrp::multi_usrp::make(d_tx_args);
    d_usrp->set_tx_freq(d_tx_freq);
    d_usrp->set_rx_freq(d_rx_freq);
    while (not d_usrp->get_rx_sensor("lo_locked").to_bool()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    d_usrp->set_tx_rate(d_samp_rate);
    d_usrp->set_rx_rate(d_samp_rate);

    d_usrp->set_tx_gain(d_tx_gain);
    d_usrp->set_rx_gain(d_rx_gain);

    d_pulse_count = 0;
    d_sample_count = 0;
    d_meta = pmt::make_dict();

    read_calibration_json();
}

/*
 * Our virtual destructor.
 */
usrp_radar_impl::~usrp_radar_impl() {}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        // Maintain any metadata that was produced by upstream blocks
        d_meta = pmt::car(msg);
        // Parse the metadata to update waveform parameters
        pmt::pmt_t new_prf = pmt::dict_ref(d_meta, PMT_PRF, pmt::PMT_NIL);
        if (not pmt::is_null(new_prf)) {
            d_prf = pmt::to_double(new_prf);
        }


        // Append additional metadata to the pmt object
        d_meta = pmt::dict_add(d_meta, PMT_FREQUENCY, pmt::from_double(d_tx_freq));
        d_armed = true;
        gr::thread::scoped_lock lock(d_tx_buff_mutex);
        d_tx_buff = c32vector_elements(pmt::cdr(msg));
        // Need to append some zeros to the front to account for the front of
        // the waveform being cut off on the X310. This was found to be about
        // 1.5us of data, regardless of the sample rate or master clock rate
        std::vector<gr_complex> start_zeros(round(d_samp_rate * BURST_MODE_DELAY), 0);
        d_tx_buff.insert(d_tx_buff.begin(), start_zeros.begin(), start_zeros.end());
    }
}

void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                               std::vector<std::complex<float>*> buff_ptrs,
                               size_t num_samp_pulse,
                               uhd::time_spec_t start_time)
{
    if (d_tx_thread_priority != 0) {
        uhd::set_thread_priority_safe(d_tx_thread_priority);
    }
    uhd::stream_args_t tx_stream_args("fc32", "sc16");
    uhd::tx_streamer::sptr tx_stream;
    tx_stream_args.channels.push_back(0);
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    // Create metadata structure
    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst = false;
    tx_md.has_time_spec = (start_time.get_real_secs() > 0 ? true : false);
    tx_md.time_spec = start_time;

    // Send the transmit buffer continuously. Note that this cannot be done in
    // bursts because there is a bug on the X310 that chops off part of the
    // waveform at the beginning of each burst

    std::vector<gr_complex> zeros(num_samp_pulse / 10, 0);
    while (!d_finished) {
        // Update the waveform data if it has changed
        if (d_armed) {
            d_armed = false;
            d_meta =
                pmt::dict_add(d_meta, PMT_SAMPLE_START, pmt::from_uint64(d_sample_count));
            gr::thread::scoped_lock lock(d_tx_buff_mutex);
            for (size_t i = 0; i < buff_ptrs.size(); i++) {
                buff_ptrs[i] = d_tx_buff.data();
            }
            num_samp_pulse = d_tx_buff.size();
        }
        boost::this_thread::disable_interruption disable_interrupt;
        tx_md.start_of_burst = true;
        tx_md.end_of_burst = false;
        tx_md.has_time_spec = true;
        tx_stream->send(buff_ptrs, num_samp_pulse, tx_md, 0.1);
        // Send a mini EOB to tell the USRP that we're done
        tx_md.start_of_burst = false;
        tx_md.end_of_burst = true;
        tx_md.has_time_spec = false;
        tx_stream->send(zeros.data(), zeros.size(), tx_md);
        boost::this_thread::restore_interruption restore_interrupt(disable_interrupt);


        tx_md.time_spec += 1 / d_prf;
        d_pulse_count++;
        d_sample_count += num_samp_pulse;
    }
}

void usrp_radar_impl::receive(uhd::usrp::multi_usrp::sptr usrp,
                              std::vector<std::vector<gr_complex>> buffs,
                              uhd::time_spec_t start_time)
{
    if (d_rx_thread_priority != 0) {
        uhd::set_thread_priority_safe(d_tx_thread_priority);
    }
    size_t channels = buffs.size();
    std::vector<size_t> channel_vec;
    uhd::stream_args_t stream_args("fc32", "sc16");
    uhd::rx_streamer::sptr rx_stream;
    if (channel_vec.size() == 0) {
        for (size_t i = 0; i < channels; i++) {
            channel_vec.push_back(i);
        }
    }
    stream_args.channels = channel_vec;
    rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    stream_cmd.stream_now = (start_time.get_real_secs() > 0 ? false : true);
    stream_cmd.time_spec = start_time + BURST_MODE_DELAY;
    rx_stream->issue_stream_cmd(stream_cmd);

    size_t max_num_samps = rx_stream->get_max_num_samps();
    size_t num_samps_total = 0;
    size_t num_samps_pri = buffs[0].size();

    std::vector<gr_complex*> buff_ptrs2(buffs.size());
    d_rx_data = pmt::make_c32vector(num_samps_pri, 0);
    double timeout = 0.5 + start_time.get_real_secs();

    while (!d_finished) {
        // Move storing pointer to correct location
        for (size_t i = 0; i < channels; i++)
            buff_ptrs2[i] = &(buffs[i][num_samps_total]);

        // Sampling data
        size_t samps_to_recv = std::min(num_samps_pri - num_samps_total, max_num_samps);
        boost::this_thread::disable_interruption disable_interrupt;
        size_t num_rx_samps = rx_stream->recv(buff_ptrs2, samps_to_recv, md, timeout);
        boost::this_thread::restore_interruption restore_interrupt(disable_interrupt);

        timeout = 0.5;

        num_samps_total += num_rx_samps;
        // Account for the inherent delay in the USRPs (the number of samples is
        // pulled from the calibration file). This delay is only deterministic
        // if the transmit and receive operations were started with a timed
        // command, so don't apply them if the user decides to start streaming ASAP.
        if (not stream_cmd.stream_now and d_delay_samps > 0) {
            num_samps_total -= d_delay_samps;
            d_delay_samps = 0;
        }

        // TODO: Handle errors
        // if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
        //     break;
        // if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)
        //     break;
        // Send the pdu for the PRI
        if (num_samps_total == num_samps_pri) {
            // Copy the received data into the message PDU and send it
            gr_complex* ptr = pmt::c32vector_writable_elements(d_rx_data, num_samps_pri);
            std::copy(buffs[0].begin(), buffs[0].end(), ptr);
            message_port_pub(PMT_OUT, pmt::cons(d_meta, d_rx_data));
            // Reset the metadata
            d_meta = pmt::make_dict();
            num_samps_total = 0;
            // If the PRF has changed, resize the buffers
            // size_t tx_buff_size;
            // TODO: Account for this in the metadata
            // {
            //     gr::thread::scoped_lock lock(d_tx_buff_mutex);
            //     tx_buff_size = d_tx_buff.size();
            // }
            // if (tx_buff_size != num_samps_pri) {
            //     num_samps_pri = tx_buff_size;
            //     for (size_t i = 0; i < buffs.size(); i++) {
            //         buffs[i].resize(num_samps_pri);
            //     }
            // }
        }
    }


    // Shut down the stream
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);
}

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

void usrp_radar_impl::run()
{
    while (d_tx_buff.size() == 0) {
        // Wait for data to arrive
        boost::this_thread::sleep(boost::posix_time::microseconds(1));
    }
    // Set up Tx buffer
    std::vector<gr_complex*> tx_buff_ptrs;
    tx_buff_ptrs.push_back(&d_tx_buff.front());

    // Set up Rx buffer
    size_t num_samp_rx = round(d_samp_rate / d_prf);
    std::vector<gr_complex*> rx_buff_ptrs;
    d_rx_buff = std::vector<gr_complex>(num_samp_rx, 0);
    rx_buff_ptrs.push_back(&d_rx_buff.front());
    std::vector<std::vector<gr_complex>> rx_buffs;
    rx_buffs.push_back(d_rx_buff);

    // Start the transmit and receive threads
    // If a time in the future is given
    uhd::time_spec_t time_now = uhd::time_spec_t(0.0);
    const pmt::pmt_t outport = PMT_OUT;
    if (d_start_time.get_real_secs() != 0) {
        time_now = d_usrp->get_time_now();
    }
    d_tx_thread = gr::thread::thread([this, tx_buff_ptrs, time_now] {
        transmit(d_usrp, tx_buff_ptrs, d_tx_buff.size(), time_now + d_start_time);
    });
    // Start receiving in the main thread
    receive(d_usrp, rx_buffs, time_now + d_start_time);
    // Wait for Tx and Rx to finish the current pulse
    d_tx_thread.join();
}

void usrp_radar_impl::read_calibration_json()
{
    // const std::string homedir = getenv("HOME");
    const std::string homedir = "/home/shane";
    std::ifstream file(homedir + "/.uhd/delay_calibration.json");
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
            UHD_LOG_INFO("RadarWindow",
                         "Calibration file found, but no data exists for this "
                         "combination of radio, master clock rate, and sample rate");
    } else {
        UHD_LOG_INFO("RadarWindow", "No calibration file found");
    }

    file.close();
}

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
