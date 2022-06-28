#ifndef C74FE057_CBE3_4619_B18E_7A7AE942711F
#define C74FE057_CBE3_4619_B18E_7A7AE942711F

#include <Eigen/Dense>
#include <volk/volk_alloc.hh>
#include <QEvent>
#include <vector>

static constexpr int RadarUpdateEventType = 4096;

class RangeDopplerUpdateEvent : public QEvent
{
public:
    RangeDopplerUpdateEvent(const Eigen::ArrayXd data);
    ~RangeDopplerUpdateEvent() override;
    const double* data() const;
    static QEvent::Type Type() { return QEvent::Type(RadarUpdateEventType); }

private:
  Eigen::ArrayXd d_data;
};

#endif /* C74FE057_CBE3_4619_B18E_7A7AE942711F */
