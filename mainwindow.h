#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QVector>
#include "qcustomplot.h"

namespace Ui {
class MainWindow;
}

typedef std::pair<QVector<double>,QVector<double>> hist;

struct DetectorConfig {
  unsigned long int noiseLevel      = 0;
  unsigned long int loopsPerReport  = 1024*128;
  unsigned long int reportsPerReset = 32;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

    DetectorConfig configLive, configInput;
    bool configInputSet = false;
    QUdpSocket *socket;
    QCPBars *plot;

    QVector<int> peakData = QVector<int>(4096);
    QVector<double> plotValues = QVector<double>(4096);
    QVector<double> plotKeys = QVector<double>(4096);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushConnect_clicked();
    void readDatagram();

    void on_pushApply_clicked();

    void on_pushReset_clicked();

    void on_spinMIPNew_valueChanged(int arg1);

    void on_spinMIRNew_valueChanged(int arg1);

    void on_spinMIntNew_valueChanged(int arg1);

    void on_spinNoiseNew_valueChanged(double arg1);

    void on_checkNoiseNew_clicked(bool checked);

    void on_spinLNLNew_valueChanged(int arg1);

    void on_spinHNLNew_valueChanged(int arg1);

    void on_saveButton_clicked();

private:
    Ui::MainWindow *ui;

    void parseHandshake();
    void parseKeepAlive();
    void parseConfig(const QNetworkDatagram& datagram);
    void parseData(const QNetworkDatagram& datagram);

    void sendHandshake();
    void sendConfig();

    void updateStats();

    void setupPlot();
    void updatePlot();

    int rebinCounter = 0;

    hist rebin(const QVector<double> &keys, const QVector<double> &values, int n);
};

#endif // MAINWINDOW_H
