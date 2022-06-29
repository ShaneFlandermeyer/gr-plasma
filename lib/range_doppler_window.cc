#include "range_doppler_window.h"
#include <iostream>

RangeDopplerWindow::RangeDopplerWindow(QWidget* parent) : QWidget(parent)
{
    // Debug Plot
    // for (int index = 0; index < 100; ++index) {
    //     xData[index] = index;
    //     yData[index] = 50;
    // }
    d_debug_curve = new QwtPlotCurve();
    d_debug_plot = new QwtPlot();
    // d_debug_curve->setSamples(xData, yData, 100);
    d_debug_curve->attach(d_debug_plot);

    // Spectrogram
    d_spectro = new QwtPlotSpectrogram();
    d_plot = new QwtPlot();
    d_spectro->attach(d_plot);

    // GUI layout
    vLayout = new QVBoxLayout();
    vLayout->addWidget(d_debug_plot);
    vLayout->addWidget(d_plot);
    setLayout(vLayout);

    d_closed = false;
}

RangeDopplerWindow::~RangeDopplerWindow() { d_closed = true; }

bool RangeDopplerWindow::is_closed() const { return d_closed; }

void RangeDopplerWindow::customEvent(QEvent* e)
{
    if (e->type() == RangeDopplerUpdateEvent::Type()) {
        RangeDopplerUpdateEvent* event = (RangeDopplerUpdateEvent*)e;
        auto* data = event->data();
        auto nrow = event->nrow();
        auto ncol = event->ncol();
        
        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> tmp(nrow,ncol);
        // std::copy(tmp.data(), data, tmp.size()*sizeof(std::complex<float>));
        for (auto i = 0; i < tmp.size(); i++) {
            tmp.data()[i] = abs(data[i]);
        //     // std::cout << tmp.data()[i];
        }
        tmp.transposeInPlace();
        
        QVector<double> vec(tmp.size());
        std::copy(tmp.data(), tmp.data() + tmp.size(), vec.data());
        QwtMatrixRasterData* mrd = new QwtMatrixRasterData();
        mrd->setInterval(Qt::XAxis, QwtInterval(0, tmp.cols()));
        mrd->setInterval(Qt::YAxis, QwtInterval(0, tmp.rows()));
        mrd->setInterval(Qt::ZAxis, QwtInterval(tmp.minCoeff(), tmp.maxCoeff()));
        mrd->setValueMatrix(vec, ncol);
        d_spectro->setData(mrd);
        d_plot->replot();
        // size_t n = event->numPoints();
        // std::vector<double> x(n);
        // std::vector<double> y(n);
        // for (size_t i = 0; i < n; i++)
        //     x[i] = i;

        // memcpy(y.data(), data, n * sizeof(data[0]));
        // d_debug_curve->setSamples(x.data(), y.data(), n);
        // d_debug_plot->setAutoReplot();

        // d_debug_plot->replot();
        // d_debug_plot->axisAutoScale(QwtPlot::yLeft);
    }


    // double pulse_width = 20e-6;
    // double samp_rate = 40e6;
    // double bandwidth = 0.75 * samp_rate;
    // double prf = 5e3;
    // size_t num_pulse_cpi = 1024;
    // size_t num_samp_pri = samp_rate / prf;
    // size_t num_samp_cpi = num_samp_pri * num_pulse_cpi;
    // plasma::LinearFMWaveform waveform(bandwidth, pulse_width, 0, samp_rate);
    // Eigen::ArrayXcf mf = waveform.MatchedFilter().cast<std::complex<float>>();
    // // Load the waveform received by the SDR
    // std::vector<std::complex<float>> data =
    //     plasma::read<std::complex<float>>("/home/shane/test.dat", 0, num_samp_cpi);
    // // Reshape the data into pulses
    // Eigen::ArrayXXcf fast_time_slow_time =
    //     Eigen::Map<Eigen::ArrayXXcf>(data.data(), data.size(), 1);
    // fast_time_slow_time.resize(fast_time_slow_time.rows() / num_pulse_cpi,
    // num_pulse_cpi);

    // // Do range processing
    // int num_thread = 10;
    // Eigen::ArrayXXcf range_pulse_map = plasma::conv(fast_time_slow_time, mf,
    // num_thread);

    // // Do doppler processing
    // Eigen::ArrayXXcf range_dopp_map(range_pulse_map.rows(), range_pulse_map.cols());
    // plasma::FFT<std::complex<float>, true> fft(range_pulse_map.cols(), num_thread);
    // for (int i = 0; i < range_pulse_map.rows(); i++) {
    //     Eigen::ArrayXcf row = range_pulse_map.row(i);
    //     range_dopp_map.row(i) = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
    //         fft.execute(row.data()), row.size());
    // }

    // // Do an fftshift
    // Eigen::ArrayXXcf rdm = plasma::fftshift(range_dopp_map, 1);
    // Eigen::ArrayXXf rdm_db = 20 * log10(abs(rdm));
    // rdm_db = rdm_db - rdm_db.maxCoeff();
    // rdm_db = rdm_db.min(0).max(-80);
    // std::cout << "Event processed" << std::endl;
}