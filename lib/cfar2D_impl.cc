/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cfar2D_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {


cfar2D::sptr cfar2D::make(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa)
{
    return gnuradio::make_block_sptr<cfar2D_impl>(guard_cells, training_cells, pfa);
}


/*
 * The private constructor
 */
cfar2D_impl::cfar2D_impl(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa)
    : gr::block("cfar2D",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
                outPort(pmt::mp("Output"))
{   
    //Setting guard window
    guard_win.at(0) = guard_cells[0];
    guard_win.at(1) = guard_cells[1];

    //Setting training window
    train_win.at(0) = training_cells[0];
    train_win.at(1) = training_cells[1];

    //Setting PFA
    _pfa = pfa;

    //Creating the cfar Object
    cfarTemp = ::plasma::CFARDetector2D(guard_win, train_win, _pfa);

    //Setting the message ports and handler
    message_port_register_out(outPort);

    message_port_register_in(pmt::mp("Input"));
    set_msg_handler(pmt::mp("Input"),
                    [this](const pmt::pmt_t &msg) {this->recieveMessage(msg);});
}

/*
 * Our virtual destructor.
 */
cfar2D_impl::~cfar2D_impl() {}

void cfar2D_impl::recieveMessage(const pmt::pmt_t &msg){
    
    pmt::pmt_t samples;
    if(pmt::is_pdu(msg)){
        samples = pmt::cdr(msg);
    }

    size_t n = pmt::length(samples);
    size_t io(0);

    int num_pulse_cpi = 512; //Hard coded this. Need to add another parameter later for this.
    int ncol = 512;
    int nrow = n / ncol;
    const gr_complex* in = pmt::c32vector_elements(samples, io);
    af::array rdm(af::dim4(nrow, ncol), reinterpret_cast<const af::cfloat *>(in));
    // Mag squared
    rdm = af::sqrt(af::pow(rdm,2));
    // Run CFAR
    DetectionReport results = cfarTemp.detect(rdm);
    // Output detections


}



} /* namespace plasma */
} /* namespace gr */
