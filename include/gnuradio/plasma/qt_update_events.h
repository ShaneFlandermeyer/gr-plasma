#ifndef C74FE057_CBE3_4619_B18E_7A7AE942711F
#define C74FE057_CBE3_4619_B18E_7A7AE942711F

#include <QEvent>
#include <vector>
#include <complex>

static constexpr int RadarUpdateEventType = 4096;

class RangeDopplerUpdateEvent : public QEvent
{
public:
    RangeDopplerUpdateEvent(const double* data, size_t rows, size_t cols);
    ~RangeDopplerUpdateEvent() override;
    const double* data() const;
    const size_t cols();
    const size_t rows();
    static QEvent::Type Type() { return QEvent::Type(RadarUpdateEventType); }

private:
  const double* d_data;
  size_t d_rows;
  size_t d_cols;
  
};

#endif /* C74FE057_CBE3_4619_B18E_7A7AE942711F */
