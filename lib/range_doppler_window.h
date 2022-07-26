#ifndef C63B8235_0BB0_46FF_A644_A4CCB87E809D
#define C63B8235_0BB0_46FF_A644_A4CCB87E809D

#include <gnuradio/plasma/qt_update_events.h>
#include <plasma_dsp/file.h>
#include <plasma_dsp/linear_fm_waveform.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <pmt/pmt.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_thermo.h>
#include <qwt_color_map.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <QBoxLayout>
#include <QWidget>
#include <complex>
#include <vector>

class RangeDopplerWindow : public QWidget
{
    Q_OBJECT

public:
    RangeDopplerWindow(QWidget* parent = nullptr);
    ~RangeDopplerWindow();

    bool is_closed() const;
    bool busy() const;

    void xlim(double x1, double x2);
    void ylim(double y1, double y2);

public slots:
    void customEvent(QEvent* e) override;

private:
    bool d_closed;
    QwtPlotSpectrogram* d_spectro;
    QwtPlot* d_debug_plot;
    QwtPlot* d_plot;
    QwtPlotCurve* d_debug_curve;
    QwtMatrixRasterData* d_data;
    QwtPlotZoomer* d_zoomer;
    QwtPlotPanner* d_panner;

    QVBoxLayout* vLayout;
    QHBoxLayout* hLayout;

    double d_prf;
    double d_samp_rate;
    double d_pulsewidth;
    double d_center_freq;
    std::atomic<bool> d_busy;
};

#endif /* C63B8235_0BB0_46FF_A644_A4CCB87E809D */
