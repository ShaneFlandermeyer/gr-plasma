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

usrp_radar::sptr usrp_radar::make(const std::string& args,
                                  const double tx_rate,
                                  const double rx_rate,
                                  const double tx_freq,
                                  const double rx_freq,
                                  const double tx_gain,
                                  const double rx_gain,
                                  const double start_delay,
                                  const bool elevate_priority,
                                  const std::string& cal_file,
                                  const bool verbose)
{
    return gnuradio::make_block_sptr<usrp_radar_impl>(args,
                                                      tx_rate,
                                                      rx_rate,
                                                      tx_freq,
                                                      rx_freq,
                                                      tx_gain,
                                                      rx_gain,
                                                      start_delay,
                                                      elevate_priority,
                                                      cal_file,
                                                      verbose);
}

usrp_radar_impl::usrp_radar_impl(const std::string& args,
                                 const double tx_rate,
                                 const double rx_rate,
                                 const double tx_freq,
                                 const double rx_freq,
                                 const double tx_gain,
                                 const double rx_gain,
                                 const double start_delay,
                                 const bool elevate_priority,
                                 const std::string& cal_file,
                                 const bool verbose)
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      usrp_args(args),
      tx_rate(tx_rate),
      rx_rate(rx_rate),
      tx_freq(tx_freq),
      rx_freq(rx_freq),
      tx_gain(tx_gain),
      rx_gain(rx_gain),
      start_delay(start_delay),
      elevate_priority(elevate_priority),
      calibration_file(cal_file),
      verbose(verbose)
{
    // Additional parameters. I have the hooks in to make them configurable, but we don't
    // need them right now.
    this->tx_subdev = this->rx_subdev = "";
    this->tx_cpu_format = this->rx_cpu_format = "fc32";
    this->tx_otw_format = this->rx_otw_format = "sc16";
    this->tx_device_addr = this->rx_device_addr = "";
    this->tx_channel_nums = std::vector<size_t>(1, 0);
    this->rx_channel_nums = std::vector<size_t>(1, 0);
    this->tx_buffs = std::vector<const void*>(tx_channel_nums.size(), nullptr);

    this->n_tx_total = 0;
    this->new_msg_received = false;
    this->next_meta = pmt::make_dict();

    config_usrp(this->usrp,
                this->usrp_args,
                this->tx_rate,
                this->rx_rate,
                this->tx_freq,
                this->rx_freq,
                this->tx_gain,
                this->rx_gain,
                this->tx_subdev,
                this->rx_subdev,
                this->verbose);

    n_delay = 0;
    if (not cal_file.empty()) {
        read_calibration_file(cal_file);
    }

    message_port_register_in(PMT_IN);
    message_port_register_out(PMT_OUT);
    set_msg_handler(PMT_IN, [this](pmt::pmt_t msg) { this->handle_message(msg); });
}

usrp_radar_impl::~usrp_radar_impl() {}


bool usrp_radar_impl::start()
{
    finished = false;
    main_thread = gr::thread::thread(&usrp_radar_impl::run, this);
    return block::start();
}

bool usrp_radar_impl::stop()
{
    finished = true;
    return block::stop();
}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        next_meta = pmt::dict_update(next_meta, pmt::car(msg));
        tx_data = pmt::cdr(msg);
        tx_buff_size = pmt::length(tx_data);

        new_msg_received = true;
    }
}

void usrp_radar_impl::run()
{
    while (not new_msg_received) {
        if (finished) {
            return;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }


    double start_time = usrp->get_time_now().get_real_secs() + start_delay;
    /***********************************************************************
     * Receive thread
     **********************************************************************/

    // create a receive streamer
    uhd::stream_args_t rx_stream_args(rx_cpu_format, rx_otw_format);
    rx_stream_args.channels = rx_channel_nums;
    rx_stream_args.args = uhd::device_addr_t(rx_device_addr);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(rx_stream_args);
    rx_thread =
        gr::thread::thread(&usrp_radar_impl::receive, this, usrp, rx_stream, start_time);

    /***********************************************************************
     * Transmit thread
     **********************************************************************/
    uhd::stream_args_t tx_stream_args(tx_cpu_format, tx_otw_format);
    tx_stream_args.channels = tx_channel_nums;
    tx_stream_args.args = uhd::device_addr_t(tx_device_addr);
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(tx_stream_args);
    tx_thread =
        gr::thread::thread(&usrp_radar_impl::transmit, this, usrp, tx_stream, start_time);

    if (elevate_priority) {
        gr::thread::set_thread_priority(tx_thread.native_handle(), 99);
        gr::thread::set_thread_priority(rx_thread.native_handle(), 99);
    }

    tx_thread.join();
    rx_thread.join();
}

void usrp_radar_impl::config_usrp(uhd::usrp::multi_usrp::sptr& usrp,
                                  const std::string& args,
                                  const double tx_rate,
                                  const double rx_rate,
                                  const double tx_freq,
                                  const double rx_freq,
                                  const double tx_gain,
                                  const double rx_gain,
                                  const std::string& tx_subdev,
                                  const std::string& rx_subdev,
                                  bool verbose)
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
    usrp->set_tx_gain(tx_gain);
    usrp->set_rx_gain(rx_gain);

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
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp->get_tx_gain()
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp->get_rx_gain()
                  << std::endl;
    }
}

void usrp_radar_impl::receive(uhd::usrp::multi_usrp::sptr usrp,
                              uhd::rx_streamer::sptr rx_stream,
                              double start_time)
{

    // setup variables
    uhd::rx_metadata_t md;
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.time_spec = uhd::time_spec_t(start_time);
    cmd.stream_now = start_time >= usrp->get_time_now().get_real_secs();
    rx_stream->issue_stream_cmd(cmd);

    // Set up and allocate buffers
    pmt::pmt_t rx_data_pmt = pmt::make_c32vector(tx_buff_size, 0);
    gr_complex* rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, tx_buff_size);

    double time_until_start = start_time - usrp->get_time_now().get_real_secs();
    double timeout = 0.1 + time_until_start;
    bool stop_called = false;

    // TODO: Handle multiple channels (e.g., one out port per channel)
    while (true) {
        if (finished and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        try {

            if (n_delay > 0) {
                // Throw away n_delay samples at the beginning
                std::vector<gr_complex> dummy_vec(n_delay);
                n_delay -= rx_stream->recv(dummy_vec.data(), n_delay, md, timeout);
            }
            rx_stream->recv(rx_data_ptr, tx_buff_size, md, timeout);
            timeout = 0.1;

            // Copy any new metadata to the output and reset the metadata
            pmt::pmt_t meta = this->next_meta;
            if (pmt::length(meta) > 0) {
                meta = pmt::dict_add(
                    meta, pmt::intern(rx_freq_key), pmt::from_double(rx_freq));
                this->next_meta = pmt::make_dict();
            }
            message_port_pub(PMT_OUT, pmt::cons(meta, rx_data_pmt));

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

void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                               uhd::tx_streamer::sptr tx_stream,
                               double start_time)
{

    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = start_time != 0.0;
    md.time_spec = uhd::time_spec_t(start_time);

    double timeout = 0.1 + start_time;
    while (not finished) {
        if (new_msg_received) {
            tx_buffs[0] = pmt::c32vector_writable_elements(tx_data, tx_buff_size);
            next_meta = pmt::dict_add(
                next_meta, pmt::intern(tx_freq_key), pmt::from_double(tx_freq));
            next_meta = pmt::dict_add(
                next_meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));
            new_msg_received = false;
        }
        n_tx_total += tx_stream->send(tx_buffs, tx_buff_size, md, timeout) *
                      tx_stream->get_num_channels();
        md.has_time_spec = false;
        timeout = 0.1;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
}

void usrp_radar_impl::read_calibration_file(const std::string& filename)
{
    std::ifstream file(filename);
    nlohmann::json json;
    if (file) {
        file >> json;
        std::string radio_type = usrp->get_mboard_name();
        for (auto& config : json[radio_type]) {
            if (config["samp_rate"] == usrp->get_tx_rate() and
                config["master_clock_rate"] == usrp->get_master_clock_rate()) {
                n_delay = config["delay"];
                break;
            }
        }
        if (n_delay == 0)
            UHD_LOG_INFO("USRP Radar",
                         "Calibration file found, but no data exists for this "
                         "combination of radio, master clock rate, and sample rate");
    } else {
        UHD_LOG_INFO("USRP Radar", "No calibration file found");
    }

    file.close();
}

void usrp_radar_impl::set_metadata_keys(const std::string& tx_freq_key,
                                        const std::string& rx_freq_key,
                                        const std::string& sample_start_key)
{
    this->tx_freq_key = tx_freq_key;
    this->rx_freq_key = rx_freq_key;
    this->sample_start_key = sample_start_key;
}

} /* namespace plasma */
} /* namespace gr */