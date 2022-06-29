#include <gnuradio/plasma/qt_update_events.h>
#include <iostream>
RangeDopplerUpdateEvent::RangeDopplerUpdateEvent(double* data, size_t rows, size_t cols)
    : QEvent(QEvent::Type(RadarUpdateEventType))
{
    d_rows = rows;
    d_cols = cols;
    d_data = new double[rows * cols];
    memcpy(d_data, data, rows * cols * sizeof(double));
}

RangeDopplerUpdateEvent::~RangeDopplerUpdateEvent() {}

const size_t RangeDopplerUpdateEvent::cols() { return d_cols; }

const size_t RangeDopplerUpdateEvent::rows() { return d_rows; }

double* RangeDopplerUpdateEvent::data() { return d_data; }