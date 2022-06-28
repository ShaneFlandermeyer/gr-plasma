#include <gnuradio/plasma/qt_update_events.h>

RangeDopplerUpdateEvent::RangeDopplerUpdateEvent(const Eigen::ArrayXd data,
                                                 size_t num_points)
    : QEvent(QEvent::Type(RadarUpdateEventType))
{
    d_data = data;
    d_num_points = num_points;
}

RangeDopplerUpdateEvent::~RangeDopplerUpdateEvent() {}

const size_t RangeDopplerUpdateEvent::numPoints() { return d_num_points; }

const double* RangeDopplerUpdateEvent::data() const { return d_data.data(); }