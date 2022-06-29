#ifndef C74FE057_CBE3_4619_B18E_7A7AE942711F
#define C74FE057_CBE3_4619_B18E_7A7AE942711F

#include <Eigen/Dense>
#include <volk/volk_alloc.hh>
#include <QEvent>
#include <vector>
#include <complex>

static constexpr int RadarUpdateEventType = 4096;

class RangeDopplerUpdateEvent : public QEvent
{
public:
    RangeDopplerUpdateEvent(const Eigen::ArrayXXcf data, size_t nrow, size_t ncol);
    ~RangeDopplerUpdateEvent() override;
    const std::complex<float>* data() const;
    const size_t ncol();
    const size_t nrow();
    static QEvent::Type Type() { return QEvent::Type(RadarUpdateEventType); }

private:
  Eigen::ArrayXXcf d_data;
  size_t d_num_col;
  size_t d_num_row;
};

#endif /* C74FE057_CBE3_4619_B18E_7A7AE942711F */
