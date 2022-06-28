#include <gnuradio/plasma/qt_update_events.h>

RangeDopplerUpdateEvent::RangeDopplerUpdateEvent(const Eigen::ArrayXd data)
    : QEvent(QEvent::Type(RadarUpdateEventType))
{
  d_data = data;
}

RangeDopplerUpdateEvent::~RangeDopplerUpdateEvent() {}

const double* RangeDopplerUpdateEvent::data() const {
  return d_data.data();
}