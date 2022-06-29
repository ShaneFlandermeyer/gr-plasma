#ifndef C63B8235_0BB0_46FF_A644_A4CCB87E809D
#define C63B8235_0BB0_46FF_A644_A4CCB87E809D

#include <gnuradio/plasma/qt_update_events.h>
#include <Eigen/Dense>
#include <plasma_dsp/fft.h>
#include <plasma_dsp/fftshift.h>
#include <plasma_dsp/file.h>
#include <plasma_dsp/filter.h>
#include <plasma_dsp/linear_fm_waveform.h>
#include <qwt_color_map.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_spectrogram.h>
#include <QBoxLayout>
#include <QWidget>
#include <complex>
#include <vector>
#include <qwt_scale_widget.h>

class RangeDopplerWindow : public QWidget
{
    Q_OBJECT

public:
    RangeDopplerWindow(QWidget* parent = nullptr);
    ~RangeDopplerWindow();

    bool is_closed() const;

public slots:
    void customEvent(QEvent* e) override;

private:
    bool d_closed;
    QwtPlotSpectrogram* d_spectro;
    QwtPlot* d_debug_plot;
    QwtPlot* d_plot;
    QwtPlotCurve* d_debug_curve;
    QwtMatrixRasterData* d_data;

    QVBoxLayout* vLayout;
    QHBoxLayout* hLayout;

    double xData[100];
    double yData[100];
};

#endif /* C63B8235_0BB0_46FF_A644_A4CCB87E809D */
