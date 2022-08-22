#ifndef C74FE057_CBE3_4619_B18E_7A7AE942711F
#define C74FE057_CBE3_4619_B18E_7A7AE942711F

#include <QEvent>
#include <complex>
#include <pmt/pmt.h>
#include <vector>

static constexpr int RadarUpdateEventType = 4096;

class RangeDopplerUpdateEvent : public QEvent
{
public:
    RangeDopplerUpdateEvent(const double* data,
                            size_t rows,
                            size_t cols,
                            pmt::pmt_t meta);
    ~RangeDopplerUpdateEvent() override;
    double* data();
    const size_t cols();
    const size_t rows();
    const pmt::pmt_t meta();
    static QEvent::Type Type() { return QEvent::Type(RadarUpdateEventType); }

private:
    double* d_data;
    size_t d_rows;
    size_t d_cols;
    pmt::pmt_t d_meta;
};

#endif /* C74FE057_CBE3_4619_B18E_7A7AE942711F */
