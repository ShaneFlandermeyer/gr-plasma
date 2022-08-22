#ifndef C63B8235_0BB0_46FF_A644_A4CCB87E809D
#define C63B8235_0BB0_46FF_A644_A4CCB87E809D

#include <gnuradio/plasma/pmt_constants.h>
#include <gnuradio/plasma/qt_update_events.h>
#include <plasma_dsp/file.h>
#include <plasma_dsp/linear_fm_waveform.h>
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
#include <qwt_symbol.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QWidget>

#include <complex>
#include <iostream>
#include <vector>

class RangeDopplerData : public QwtMatrixRasterData
{
public:
    RangeDopplerData() : QwtMatrixRasterData() {}

    virtual void setValueMatrix(const QVector<double>& values, int numColumns)
    {
        this->values = values;
        this->numColumns = numColumns;
        numRows = values.size() / numColumns;

        const QwtInterval xInterval = interval(Qt::XAxis);
        const QwtInterval yInterval = interval(Qt::YAxis);
        if (xInterval.isValid())
            dx = xInterval.width() / numColumns;
        if (yInterval.isValid())
            dy = yInterval.width() / numRows;
    }

    virtual void setInterval(Qt::Axis axis, const QwtInterval& interval)
    {
        QwtRasterData::setInterval( axis, interval );
        if (axis == Qt::XAxis) {
            dx = interval.width() / numColumns;
        } else if (axis == Qt::YAxis) {
            dy = interval.width() / numRows;
        }
    }

    virtual double value(double x, double y) const
    {
        const QwtInterval xInterval = interval(Qt::XAxis);
        const QwtInterval yInterval = interval(Qt::YAxis);

        if (!(xInterval.contains(x) && yInterval.contains(y)))
            return qQNaN();

        double value;

        int row = round((y - yInterval.minValue()) / dy);
        int col = round((x - xInterval.minValue()) / dx);

        if (row >= numRows)
            row = numRows - 1;

        if (col >= numColumns)
            col = numColumns - 1;

        value = values[row * numColumns + col];

        return value;
    }

private:
    QVector<double> values;
    int numColumns;
    int numRows;
    double dx;
    double dy;
};

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

    void show_detections(bool checked);

private:
    bool d_closed;
    QwtPlotSpectrogram* d_spectro;
    QwtPlot* d_debug_plot;
    QwtPlot* d_plot;
    QwtPlotCurve* d_debug_curve;
    RangeDopplerData* d_data;
    QwtPlotZoomer* d_zoomer;
    QwtPlotPanner* d_panner;

    // CFAR Stuff
    QCheckBox* d_checkBox;
    QwtPlotCurve* d_curve;

    QVBoxLayout* v_layout;
    QHBoxLayout* h_layout;

    double d_prf;
    double d_samp_rate;
    double d_pulsewidth;
    double d_center_freq;
    std::atomic<bool> d_busy;
};

#endif /* C63B8235_0BB0_46FF_A644_A4CCB87E809D */
