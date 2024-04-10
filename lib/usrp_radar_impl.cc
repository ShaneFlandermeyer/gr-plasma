/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "usrp_radar_impl.h"
#include <gnuradio/io_signature.h>

float BURST_WARMUP_TIME = 3e-6;
float BURST_COOLDOWN_TIME = 0.5e-6;

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
    this->tx_buffs = std::vector<std::vector<gr_complex>>(tx_channel_nums.size());

    this->n_tx_total = 0;
    this->new_msg_received = false;
    this->meta = pmt::make_dict();

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
        meta = pmt::dict_update(meta, pmt::car(msg));
        tx_data = pmt::cdr(msg);
        tx_buff_size = pmt::length(tx_data);

        if (pmt::dict_has_key(meta, pmt::intern(prf_key))) {
            prf = pmt::to_double(
                pmt::dict_ref(meta, pmt::intern(prf_key), pmt::from_double(0.0)));

            if (prf == 0) {
                rx_buff_size = tx_buff_size;
            } else {
                rx_buff_size = round(rx_rate / prf);
            }
        }


        new_msg_received = true;
    }
}

void usrp_radar_impl::run()
{
    while (not new_msg_received) { // Wait for Tx data
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
    if (prf > 0) {
        tx_thread = gr::thread::thread(
            &usrp_radar_impl::transmit_bursts, this, usrp, tx_stream, start_time);
    } else {
        tx_thread = gr::thread::thread(
            &usrp_radar_impl::transmit_continuous, this, usrp, tx_stream, start_time);
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

    usrp->set_time_now(uhd::time_spec_t(0.0));

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
    if (elevate_priority) {
        uhd::set_thread_priority(1.0);
    }
    // setup variables
    uhd::rx_metadata_t md;
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.time_spec = uhd::time_spec_t(start_time);
    cmd.stream_now = usrp->get_time_now().get_real_secs() >= start_time;
    rx_stream->issue_stream_cmd(cmd);

    // Set up and allocate buffers
    pmt::pmt_t rx_data_pmt = pmt::make_c32vector(rx_buff_size, 0);
    gr_complex* rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, rx_buff_size);

    double time_until_start = start_time - usrp->get_time_now().get_real_secs();
    double timeout = 0.1 + time_until_start;

    // TODO: Handle multiple channels (e.g., one out port per channel)
    while (true) {
        if (finished) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        }

        if (n_delay > 0) {
            // Throw away n_delay samples at the beginning
            std::vector<gr_complex> dummy_vec(n_delay);
            n_delay -= rx_stream->recv(dummy_vec.data(), n_delay, md, timeout);
        }
        rx_stream->recv(rx_data_ptr, rx_buff_size, md, timeout);
        timeout = 0.1;

        if (pmt::length(this->meta) > 0) {
            this->meta = pmt::dict_add(
                this->meta, pmt::intern(rx_freq_key), pmt::from_double(rx_freq));
        }
        message_port_pub(PMT_OUT, pmt::cons(this->meta, rx_data_pmt));
        this->meta = pmt::make_dict();

        if (finished and md.end_of_burst) {
            return;
        }
    }
}

void usrp_radar_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp,
                                      uhd::tx_streamer::sptr tx_stream,
                                      double start_time)
{
    if (elevate_priority) {
        uhd::set_thread_priority(1.0);
    }
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    std::vector<gr_complex> cooldown_zeros(round(BURST_COOLDOWN_TIME * tx_rate),
                                           gr_complex(0, 0));
    std::vector<gr_complex> warmup_zeros(round(BURST_WARMUP_TIME * tx_rate),
                                         gr_complex(0, 0));
    while (not finished) {
        if (new_msg_received) {
            std::vector<gr_complex> tx_data_vector = pmt::c32vector_elements(tx_data);
            tx_buffs[0] = tx_data_vector;
            meta =
                pmt::dict_add(meta, pmt::intern(tx_freq_key), pmt::from_double(tx_freq));
            meta = pmt::dict_add(
                meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));
            new_msg_received = false;
        }
        md.start_of_burst = true;
        md.end_of_burst = false;
        md.has_time_spec = true;
        md.time_spec = uhd::time_spec_t(start_time) - BURST_WARMUP_TIME;

        double timeout = 0.1 + std::max(1 / prf, start_time);

        tx_stream->send(warmup_zeros.data(), warmup_zeros.size(), md);
        md.start_of_burst = false;
        md.has_time_spec = false;
        n_tx_total +=
            tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);

        md.end_of_burst = true;
        tx_stream->send(cooldown_zeros.data(), cooldown_zeros.size(), md);


        start_time += 1 / prf;
    }
}

void usrp_radar_impl::transmit_continuous(uhd::usrp::multi_usrp::sptr usrp,
                                          uhd::tx_streamer::sptr tx_stream,
                                          double start_time)
{
    if (elevate_priority) {
        uhd::set_thread_priority(1.0);
    }
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = true;
    md.time_spec = uhd::time_spec_t(start_time);

    double timeout = 0.1 + start_time;
    bool first = true;
    while (not finished) {
        if (new_msg_received) {
            tx_buffs[0] = pmt::c32vector_elements(tx_data);
            meta =
                pmt::dict_add(meta, pmt::intern(tx_freq_key), pmt::from_double(tx_freq));
            meta = pmt::dict_add(
                meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));
            new_msg_received = false;
        }
        n_tx_total += tx_stream->send(tx_buffs[0].data(), tx_buff_size, md, timeout) *
                      tx_stream->get_num_channels();

        if (first) {
            first = false;
            md.has_time_spec = false;
            timeout = 0.5;
        }
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
                                        const std::string& sample_start_key,
                                        const std::string& prf_key)
{
    this->tx_freq_key = tx_freq_key;
    this->rx_freq_key = rx_freq_key;
    this->sample_start_key = sample_start_key;
    this->prf_key = prf_key;
}

} /* namespace plasma */
} /* namespace gr */