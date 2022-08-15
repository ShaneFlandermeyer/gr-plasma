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
    // CFAR show/hide setup
    d_checkBox = new QCheckBox("Show CFAR");
    connect(d_checkBox,SIGNAL(toggled(bool)), this, SLOT(showCFAR(bool)), Qt::UniqueConnection);
    d_curve = new QwtPlotCurve();
    d_curve->setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol* symbol = new QwtSymbol(QwtSymbol::Diamond, 
                                      QBrush(Qt::green),
                                      QPen(Qt::black, 1),
                                      QSize(8,8));
    d_curve->setSymbol(symbol);
    
    // Spectrogram
    d_plot = new QwtPlot();
    d_spectro = new QwtPlotSpectrogram();
    d_spectro->setColorMap(new ColorMap());
    d_spectro->attach(d_plot);
    d_data = new QwtMatrixRasterData();
    d_plot->setAutoReplot();
    // QwtScaleWidget* y = d_plot->axisWidget(QwtPlot::yLeft);
    // y->setTitle("Range (m)");
    // y = d_plot->axisWidget(QwtPlot::xBottom);
    // y->setTitle("Velocity (m/s)");

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
    vLayout->addWidget(d_checkBox);
    vLayout->addWidget(d_plot);
    setLayout(vLayout);

    d_closed = false;
    d_busy = false;
    d_prf = 0;
    d_pulsewidth = 0;
    d_samp_rate = 0;
    d_center_freq = 0;
}

RangeDopplerWindow::~RangeDopplerWindow() { d_closed = true; }

bool RangeDopplerWindow::is_closed() const { return d_closed; }

bool RangeDopplerWindow::busy() const { return d_busy; }

void RangeDopplerWindow::xlim(double x1, double x2)
{
    d_data->setInterval(Qt::XAxis, QwtInterval(x1, x2));
}

void RangeDopplerWindow::ylim(double y1, double y2)
{
    d_data->setInterval(Qt::YAxis, QwtInterval(y1, y2));
}

void RangeDopplerWindow::showCFAR(bool checked){
    if (checked) {
        // show
        std::cout << "Showing CFAR" << std::endl;
        d_curve->attach(d_plot);

    } else {
        // hide
        std::cout << "Hiding CFAR" << std::endl;
        d_curve->detach();

    }
}

void RangeDopplerWindow::customEvent(QEvent* e)
{
    d_busy = true;
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
        af::array tmp(vec.size(), f64);
        tmp.write(vec.data(), sizeof(double) * vec.size());
        d_data->setInterval(Qt::ZAxis,
                            QwtInterval(af::min<double>(tmp), af::max<double>(tmp)));
        d_data->setValueMatrix(vec, cols);
        d_spectro->setData(d_data);


        const QwtInterval zInterval = d_spectro->data()->interval(Qt::ZAxis);
        QwtScaleWidget* rightAxis = d_plot->axisWidget(QwtPlot::yRight);
        rightAxis->setColorMap(zInterval, new ColorMap());
        d_plot->setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());


        pmt::pmt_t global = pmt::dict_ref(meta, PMT_GLOBAL, pmt::PMT_NIL);
        pmt::pmt_t annotation = pmt::dict_ref(meta, PMT_ANNOTATIONS, pmt::PMT_NIL);
        pmt::pmt_t capture = pmt::dict_ref(meta, PMT_CAPTURES, pmt::PMT_NIL);

        d_prf =
            pmt::to_double(pmt::dict_ref(annotation, PMT_PRF, pmt::from_double(d_prf)));
        d_pulsewidth = pmt::to_double(
            pmt::dict_ref(annotation, PMT_DURATION, pmt::from_double(d_pulsewidth)));
        d_samp_rate = pmt::to_double(
            pmt::dict_ref(global, PMT_SAMPLE_RATE, pmt::from_double(d_samp_rate)));
        d_center_freq = pmt::to_double(
            pmt::dict_ref(capture, PMT_FREQUENCY, pmt::from_double(d_center_freq)));

        

        if (d_prf == 0 or d_pulsewidth == 0 or d_samp_rate == 0) {
            ylim(0, rows);
        } else {
            const double c = ::plasma::physconst::c;
            double rmin = -(c / 2) * d_pulsewidth;
            double rmax = (c / 2) * (1 / d_prf);
            ylim(rmin, rmax);
            QwtScaleWidget* y = d_plot->axisWidget(QwtPlot::yLeft);
            y->setTitle("Range (m)");
            y = d_plot->axisWidget(QwtPlot::xBottom);
            y->setTitle("Velocity (m/s)");
        }

        if (d_center_freq == 0 or d_prf == 0) {
            xlim(0, cols);
        } else {
            const double c = ::plasma::physconst::c;
            double lam = c / d_center_freq;
            double vmax = (lam / 2) * (d_prf / 2);
            double vmin = -vmax;
            xlim(vmin, vmax);
            QwtScaleWidget* y = d_plot->axisWidget(QwtPlot::yLeft);
            y->setTitle("Range (m)");
            y = d_plot->axisWidget(QwtPlot::xBottom);
            y->setTitle("Velocity (m/s)");
        }

        // CFAR Stuff
        pmt::pmt_t indices = pmt::dict_ref(meta, pmt::mp("indices"), pmt::PMT_NIL);
        if (not pmt::is_null(indices)) {
            std::vector<int> idx = pmt::s32vector_elements(indices);
            
            //Create the x and y coordinates
            QVector<double> xData(idx.size());
            QVector<double> yData(idx.size());

            //Loop through idx array
            for (int i = 0; i < idx.size(); i++) {
                //Get the x and y coordinate with respect to result.detections array
                int detection_col = idx[i] / rows;
                int detection_row = idx[i] % rows;
                //Compute xData and yData with respect to actual spectrogram scaling, then move the "dot" to the center of the cell
                xData[i] = (detection_col + .5) / (float)cols *
               (d_data->interval(Qt::XAxis).maxValue() -
                d_data->interval(Qt::XAxis).minValue());
                yData[i] = (detection_row + .5) / (float)rows *
               (d_data->interval(Qt::YAxis).maxValue() -
                d_data->interval(Qt::YAxis).minValue());

            }

            d_curve->setSamples(xData, yData);
        }

        d_zoomer->setZoomBase(d_spectro->boundingRect());
        d_plot->replot();
    }
    d_busy = false;
}