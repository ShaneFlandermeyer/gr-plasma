#include "range_doppler_window.h"
#include <iostream>

class ColorMap : public QwtLinearColorMap
{
public:
    ColorMap() : QwtLinearColorMap(Qt::darkBlue, Qt::darkRed)
    {
        addColorStop(0.2, Qt::blue);
        addColorStop(0.4, Qt::cyan);
        addColorStop(0.6, Qt::yellow);
        addColorStop(0.8, Qt::red);
    }
};

class MyZoomer : public QwtPlotZoomer
{
public:
    MyZoomer(QWidget* canvas) : QwtPlotZoomer(canvas) { setTrackerMode(AlwaysOn); }

    virtual QwtText trackerTextF(const QPointF& pos) const
    {
        QColor bg(Qt::white);
        bg.setAlpha(200);

        QwtText text = QwtPlotZoomer::trackerTextF(pos);
        text.setBackgroundBrush(QBrush(bg));
        return text;
    }
};

RangeDopplerWindow::RangeDopplerWindow(QWidget* parent) : QWidget(parent)
{

    // Spectrogram
    d_plot = new QwtPlot();
    d_spectro = new QwtPlotSpectrogram();
    d_spectro->setColorMap(new ColorMap());
    d_spectro->attach(d_plot);
    d_data = new QwtMatrixRasterData();
    d_plot->setAutoReplot();
    QwtScaleWidget* y = d_plot->axisWidget(QwtPlot::yLeft);
    y->setTitle("Range (m)");
    y = d_plot->axisWidget(QwtPlot::xBottom);
    y->setTitle("Velocity (m/s)");

    // Colorbar setup
    QwtScaleWidget* rightAxis = d_plot->axisWidget(QwtPlot::yRight);
    rightAxis->setTitle("Intensity");
    rightAxis->setColorBarEnabled(true);
    d_plot->enableAxis(QwtPlot::yRight);

    // Plot zoomer setup
    d_zoomer = new MyZoomer(d_plot->canvas());
    d_zoomer->setMousePattern(
        QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    d_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
    const QColor c(Qt::blue);
    d_zoomer->setRubberBandPen(c);
    d_zoomer->setTrackerPen(c);
    // Plot panner setup
    d_panner = new QwtPlotPanner(d_plot->canvas());
    d_panner->setAxisEnabled(QwtPlot::yRight, false);
    d_panner->setMouseButton(Qt::MiddleButton);

    // GUI layout
    vLayout = new QVBoxLayout();
    // vLayout->addWidget(d_debug_plot);
    vLayout->addWidget(d_plot);
    setLayout(vLayout);

    d_closed = false;
    d_prf = 0;
    d_pulsewidth = 0;
    d_samp_rate = 0;
    d_center_freq = 0;
}

RangeDopplerWindow::~RangeDopplerWindow() { d_closed = true; }

bool RangeDopplerWindow::is_closed() const { return d_closed; }

void RangeDopplerWindow::xlim(double x1, double x2)
{
    d_data->setInterval(Qt::XAxis, QwtInterval(x1, x2));
}

void RangeDopplerWindow::ylim(double y1, double y2)
{
    d_data->setInterval(Qt::YAxis, QwtInterval(y1, y2));
}

void RangeDopplerWindow::customEvent(QEvent* e)
{
    if (e->type() == RangeDopplerUpdateEvent::Type()) {
        RangeDopplerUpdateEvent* event = (RangeDopplerUpdateEvent*)e;
        double* data = event->data();
        auto rows = event->rows();
        auto cols = event->cols();
        pmt::pmt_t meta = event->meta();

        // Create a new vector
        QVector<double> vec(rows * cols);
        std::copy(data, data + vec.size(), vec.data());
        // Also map the vector to an array to easily compute the minimum
        // and maximum values
        af::array tmp(vec.size(),f64);
        tmp.write(vec.data(), sizeof(double)*vec.size());
        d_data->setInterval(Qt::ZAxis, QwtInterval(af::min<double>(tmp), af::max<double>(tmp)));
        d_data->setValueMatrix(vec, cols);
        d_spectro->setData(d_data);
        d_zoomer->setZoomBase(d_spectro->boundingRect());

        const QwtInterval zInterval = d_spectro->data()->interval(Qt::ZAxis);
        QwtScaleWidget* rightAxis = d_plot->axisWidget(QwtPlot::yRight);
        rightAxis->setColorMap(zInterval, new ColorMap());
        d_plot->setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());

        d_plot->replot();

        // TODO: Update the range and doppler axes
        pmt::pmt_t prf = pmt::dict_ref(meta, PMT_PRF, pmt::PMT_NIL);
        pmt::pmt_t pulse_width = pmt::dict_ref(meta, PMT_PULSEWIDTH, pmt::PMT_NIL);
        pmt::pmt_t samp_rate = pmt::dict_ref(meta, PMT_SAMPLE_RATE, pmt::PMT_NIL);
        pmt::pmt_t center_freq = pmt::dict_ref(meta, PMT_FREQUENCY, pmt::PMT_NIL);
        if (not pmt::is_null(prf))
            d_prf = pmt::to_double(prf);
        if (not pmt::is_null(pulse_width))
            d_pulsewidth = pmt::to_double(pulse_width);
        if (not pmt::is_null(samp_rate))
            d_samp_rate = pmt::to_double(samp_rate);
        if (not pmt::is_null(center_freq))
            d_center_freq = pmt::to_double(center_freq);

        if (d_prf == 0 or d_pulsewidth == 0 or d_samp_rate == 0) {
            ylim(0, rows);
        } else {
            const double c = ::plasma::physconst::c;
            double rmin = -(c / 2) * d_pulsewidth;
            double rmax = (c / 2) * (1 / d_prf);
            ylim(rmin, rmax);
        }

        if (d_center_freq == 0 or d_prf == 0) {
            xlim(0, cols);
        } else {
            const double c = ::plasma::physconst::c;
            double lam = c / d_center_freq;
            double vmax = (lam / 2) * (d_prf / 2);
            double vmin = -vmax;
            xlim(vmin, vmax);
        }
    }
}