#include <gnuradio/plasma/qt_update_events.h>

RangeDopplerUpdateEvent::RangeDopplerUpdateEvent(const Eigen::ArrayXXcf data, size_t nrow, size_t ncol)
    : QEvent(QEvent::Type(RadarUpdateEventType))
{
    d_data = data;
    d_num_row = nrow;
    d_num_col = ncol;
}

RangeDopplerUpdateEvent::~RangeDopplerUpdateEvent() {}

const size_t RangeDopplerUpdateEvent::ncol() { return d_num_col; }

const size_t RangeDopplerUpdateEvent::nrow() { return d_num_row; }

const std::complex<float>* RangeDopplerUpdateEvent::data() const { return d_data.data(); }