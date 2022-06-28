#ifndef CB6C2392_70FE_4AAA_953F_BA548572BCC6
#define CB6C2392_70FE_4AAA_953F_BA548572BCC6
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_thermo.h>
#include <QBoxLayout>
#include <QPushButton>
#include <QWidget>

// class definition 'Window'
class Window : public QWidget
{
    // must include the Q_OBJECT macro for for the Qt signals/slots framework to work with
    // this class
    Q_OBJECT

public:
    Window(QWidget* parent = nullptr); // default constructor - called when a Window is
                                       // declared without arguments

    ~Window() { d_isClosed = true; };

    void timerEvent(QTimerEvent*);
		bool isClosed() const;

    // internal variables for the window class
    // private:
    static constexpr int plotDataSize = 100;
    static constexpr double gain = 7.5;
    bool d_isClosed = false;

    QPushButton* button;
    QwtThermo* thermo;
    QwtPlot* plot;
    QwtPlotCurve* curve;

    // layout elements from Qt itself http://qt-project.org/doc/qt-4.8/classes.html
    QVBoxLayout* vLayout; // vertical layout
    QHBoxLayout* hLayout; // horizontal layout

    // data arrays for the plot
    double xData[plotDataSize];
    double yData[plotDataSize];

    long count = 0;

    void reset();
};

#endif /* CB6C2392_70FE_4AAA_953F_BA548572BCC6 */
