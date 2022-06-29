#include <gnuradio/plasma/qt_update_events.h>

RangeDopplerUpdateEvent::RangeDopplerUpdateEvent(const double* data, size_t rows, size_t cols)
    : QEvent(QEvent::Type(RadarUpdateEventType))
{
    d_data = data;
    d_rows = rows;
    d_cols = cols;
}

RangeDopplerUpdateEvent::~RangeDopplerUpdateEvent() {}

const size_t RangeDopplerUpdateEvent::cols() { return d_cols; }

const size_t RangeDopplerUpdateEvent::rows() { return d_rows; }

const double* RangeDopplerUpdateEvent::data() const { return d_data; }