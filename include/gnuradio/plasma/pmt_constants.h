#ifndef B4AE609D_6687_4998_809D_482441F2B6F9
#define B4AE609D_6687_4998_809D_482441F2B6F9
#include <pmt/pmt.h>

// GNU-radio specific
static const pmt::pmt_t PMT_IN = pmt::intern("in");
static const pmt::pmt_t PMT_OUT = pmt::intern("out");
static const pmt::pmt_t PMT_TX = pmt::intern("tx");
static const pmt::pmt_t PMT_RX = pmt::intern("rx");
static const pmt::pmt_t PMT_PDU = pmt::intern("pdu");

// SigMF core
static const pmt::pmt_t PMT_GLOBAL = pmt::intern("global");
static const pmt::pmt_t PMT_CAPTURES = pmt::intern("captures");
static const pmt::pmt_t PMT_ANNOTATIONS = pmt::intern("annotations");
static const pmt::pmt_t PMT_DATATYPE = pmt::intern("core:datatype");
static const pmt::pmt_t PMT_VERSION = pmt::intern("core:version");
static const pmt::pmt_t PMT_SAMPLE_RATE = pmt::intern("core:sample_rate");
static const pmt::pmt_t PMT_LABEL = pmt::intern("core:label");
static const pmt::pmt_t PMT_FREQUENCY = pmt::intern("core:frequency");
static const pmt::pmt_t PMT_SAMPLE_START = pmt::intern("core:sample_start");
static const pmt::pmt_t PMT_SAMPLE_COUNT = pmt::intern("core:sample_count");

// Radar extension
static const pmt::pmt_t PMT_RADAR_DETAIL = pmt::intern("radar:detail");
static const pmt::pmt_t PMT_BANDWIDTH = pmt::intern("radar:bandwidth");
static const pmt::pmt_t PMT_DURATION = pmt::intern("radar:duration");
static const pmt::pmt_t PMT_PRF = pmt::intern("radar:prf");
static const pmt::pmt_t PMT_NUM_PULSE_CPI = pmt::intern("radar:num_pulse_cpi");
static const pmt::pmt_t PMT_DOPPLER_FFT_SIZE = pmt::intern("radar:doppler_fft_size");

#endif /* B4AE609D_6687_4998_809D_482441F2B6F9 */
