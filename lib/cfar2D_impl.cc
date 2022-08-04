/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cfar2D_impl.h"
#include <gnuradio/io_signature.h>
#include "arrayfire.h"

namespace gr {
namespace plasma {


cfar2D::sptr cfar2D::make(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa, size_t num_pulse_cpi)
{
    return gnuradio::make_block_sptr<cfar2D_impl>(guard_cells, training_cells, pfa, num_pulse_cpi);
}


/*
 * The private constructor
 */
cfar2D_impl::cfar2D_impl(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa, size_t num_pulse_cpi)
    : gr::block("cfar2D",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
                outPort(pmt::mp("Output")),
                d_num_pulse_cpi(num_pulse_cpi)
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
    //Separating the meta data to get the samples and other meta data.
    pmt::pmt_t samples;
    pmt::pmt_t meta = pmt::make_dict();
    if(pmt::is_pdu(msg)){
        samples = pmt::cdr(msg);
        meta = pmt::dict_update(meta,pmt::car(msg));
    }

    //Getting the size of the samples.
    size_t n = pmt::length(samples);
    size_t io(0);

    int ncol = d_num_pulse_cpi;
    int nrow = n / ncol;
    const gr_complex* in = pmt::c32vector_elements(samples, io);
    af::array rdm(af::dim4(nrow, ncol), reinterpret_cast<const af::cfloat *>(in));
    
    //Mag squared of rdm
    rdm = af::pow(af::abs(rdm),2);

    //Run CFAR on samples.
    DetectionReport results = cfarTemp.detect(rdm);

    //Grab data from arrayfire arrays and store them in a vector.
    int *ind_ptr = results.indices.as(s32).host<int>();
    std::vector<int> ind_vec(ind_ptr, ind_ptr + results.indices.elements());
    pmt::pmt_t indices = pmt::init_s32vector(results.indices.elements(), ind_vec);
    delete ind_ptr;

    //Adding the information to the meta data to be used later.
    meta = pmt::dict_add(meta, pmt::intern("indices"),indices);
    meta = pmt::dict_add(meta, pmt::intern("num_detections"),pmt::from_long(results.num_detections));

    //Passing the information to the next block.
    message_port_pub(outPort, pmt::cons(meta, samples));


}



} /* namespace plasma */
} /* namespace gr */
