#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <iostream>
#include <QNetworkDatagram>
#include <QIODevice>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QUdpSocket(this);
    this->ui->groupConfig->setVisible(false);
    this->ui->groupStats->setVisible(false);
    this->ui->groupPlot->setVisible(false);
    this->ui->checkNoiseCur->setChecked(true);
    setupPlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushConnect_clicked()
{
    this->ui->lineIPLocal->setEnabled(false);
    this->ui->lineIPDetector->setEnabled(false);
    this->ui->spinPortLocal->setEnabled(false);
    this->ui->spinPortDetector->setEnabled(false);

    this->ui->pushConnect->setEnabled(false);
    this->ui->pushConnect->setText("Connecting...");
    socket->bind(QHostAddress(this->ui->lineIPLocal->text()), this->ui->spinPortLocal->value());
    connect(socket, SIGNAL(readyRead()), this, SLOT(readDatagram()));
    sendHandshake();
}

void MainWindow::readDatagram()
{
    while (socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket->receiveDatagram();
        int type = *((int*)datagram.data().constData());
        switch (type) {
        case 1:
            this->parseHandshake();
            break;
        case 2:
            this->parseHandshake();
            this->parseKeepAlive();
            break;
        case 3:
            this->parseConfig(datagram);
            break;
        case 4:
            this->parseData(datagram);
            break;
        }
    }
}

void MainWindow::parseHandshake()
{
    this->ui->pushConnect->setText("Connected");
    this->ui->groupConfig->setVisible(true);
    this->ui->groupStats->setVisible(true);
    this->ui->groupPlot->setVisible(true);
    this->ui->groupConnection->setVisible(false);
}

void MainWindow::parseKeepAlive()
{

}

void MainWindow::parseConfig(const QNetworkDatagram &datagram)
{
    configLive = *(DetectorConfig*)(datagram.data().constData() + sizeof(int));

    //this->ui->spinMIPCur->setValue(this->configLive.measurementsInPeriod);
    //this->ui->spinMIRCur->setValue(this->configLive.measurementsInReport);
    //this->ui->spinMIntCur->setValue(this->configLive.measurementsInterval);

    //this->ui->spinNoiseCur->setValue(this->configLive.noiseLevel);
    //this->ui->checkNoiseCur->setChecked(this->configLive.autoNoiseLevel);
    //this->ui->spinLNLCur->setValue(this->configLive.noiseLevelMeasurementsLow);
    //this->ui->spinHNLCur->setValue(this->configLive.noiseLevelMeasurementsHigh);

    this->updateStats();
    if (!this->configInputSet) this->on_pushReset_clicked();
    this->configInputSet = true;
}

void MainWindow::parseData(const QNetworkDatagram& datagram)
{
    QVector <int> old = this->peakData;
    int Sold = this->ui->spinTotalPeaks->value();
    QFile file("D:/Energia/spectrum/spectrum.txt");
    int from = *(int*)(datagram.data().constData()+sizeof(int));
    int to   = *(int*)(datagram.data().constData()+2*sizeof(int));
    memcpy(peakData.data()+from, datagram.data().constData()+3*sizeof(int), 4*(to-from));
    if (to == 4096) {
        int S = 0;
        for(int i = 0; i < 4096; i++) {
            this->plotValues[i] = peakData[i];
            S += peakData[i];
        }
        if (S<Sold){
            for(int i = 0; i < 4096; i++) {
                char line = (char)old[i];
                if (file.open(QIODevice::Append)) {
                    file.write(&line);
                    file.write(" ");
                }
            }
        }
        file.close();
//        if (rebinCounter == 1){

                    this->plot->setData(this->plotKeys, this->plotValues);

//            hist hRebined = rebin(plotKeys,plotValues);
//            this->plot->setData(hRebined.first, hRebined.second);
            this->ui->spinTotalPeaks->setValue(S);
            this->ui->plot->rescaleAxes();
                    this->ui->plot->xAxis->setRange(0, 4096);
//            this->ui->plot->xAxis->setRange(0, 512);
            this->ui->plot->replot();
//            rebinCounter = 0;
//        }
//        rebinCounter++;
    }
}

hist MainWindow::rebin(const QVector<double> &keys, const QVector<double> &values) {
    QVector<double> newKeys(512);
    QVector<double> newValues(512,0.);
    for(int i = 0; i < 512; i++) {
        newKeys[i] = i;
        for (int j = 0; j < 8; ++j)
            newValues[i] += (double)values[i*8+j];
        newValues[i] /= 8.;
    }
    hist rebined(newKeys,newValues);
    return rebined;
}

void MainWindow::sendHandshake()
{
    char data[8];
    int type = 1;
    int dataport = this->ui->spinPortLocal->value();
    QHostAddress ip(this->ui->lineIPDetector->text());
    int port = this->ui->spinPortDetector->value();
    memcpy(data, (char*)(&type), 4);
    memcpy(data+4, (char*)(&dataport), 4);
    socket->writeDatagram(data, 8, ip, port);
}

void MainWindow::sendConfig()
{
    char data[sizeof(int) + sizeof(DetectorConfig)];
    int type = 3;
    QHostAddress ip(this->ui->lineIPDetector->text());
    int port = this->ui->spinPortDetector->value();
    memcpy(data, (char*)(&type), 4);
    memcpy(data+4, (char*)(&(this->configInput)), sizeof(DetectorConfig));
    socket->writeDatagram(data, sizeof(data), ip, port);
}

void MainWindow::updateStats()
{
    //int intervalCur = this->configLive.measurementsInterval;
    //int numberPerPeriodCur = this->configLive.measurementsInPeriod;
    //int numberPerReportCur = this->configLive.measurementsInReport;
    //double time1Cur = (intervalCur/1000.0)*numberPerPeriodCur;
    //double time2Cur = (intervalCur/1000.0)*numberPerReportCur;
    //this->ui->spinPerLenCur->setValue(time1Cur);
    //this->ui->spinRepLenCur->setValue(time2Cur);

    //int intervalNew = this->configInput.measurementsInterval;
    //int numberPerPeriodNew = this->configInput.measurementsInPeriod;
    //int numberPerReportNew = this->configInput.measurementsInReport;
    //double time1New = (intervalNew/1000.0)*numberPerPeriodNew;
    //double time2New = (intervalNew/1000.0)*numberPerReportNew;
    //this->ui->spinPerLenNew->setValue(time1New);
    //this->ui->spinRepLenNew->setValue(time2New);
}

void MainWindow::setupPlot()
{
    for (int i = 0; i < 4096; i++) {
        this->plotKeys[i] = i;
        this->plotValues[i] = 0;
    }
    this->ui->plot->xAxis->setRange(0, 1100);
    this->plot = new QCPBars(this->ui->plot->xAxis, this->ui->plot->yAxis);
    this->plot->setWidth(1);
    this->plot->setPen(Qt::NoPen);
    this->plot->setBrush(QColor(10, 140, 70, 160));
    this->plot->setData(this->plotKeys, this->plotValues);
}

void MainWindow::on_pushApply_clicked()
{
    this->sendConfig();
}

void MainWindow::on_pushReset_clicked()
{
    //this->configInput = this->configLive;
    //this->ui->spinMIPNew->setValue(this->configLive.measurementsInPeriod);
    //this->ui->spinMIRNew->setValue(this->configLive.measurementsInReport);
    //this->ui->spinMIntNew->setValue(this->configLive.measurementsInterval);

    //this->ui->spinNoiseNew->setValue(this->configLive.noiseLevel);
    //this->ui->checkNoiseNew->setChecked(this->configLive.autoNoiseLevel);
    //this->ui->spinLNLNew->setValue(this->configLive.noiseLevelMeasurementsLow);
    //this->ui->spinHNLNew->setValue(this->configLive.noiseLevelMeasurementsHigh);
}

void MainWindow::on_spinMIPNew_valueChanged(int arg1)
{
    //this->configInput.measurementsInPeriod = arg1;
    updateStats();
}

void MainWindow::on_spinMIRNew_valueChanged(int arg1)
{
    //this->configInput.measurementsInReport = arg1;
    //updateStats();
}

void MainWindow::on_spinMIntNew_valueChanged(int arg1)
{
    //this->configInput.measurementsInterval = arg1;
    //updateStats();
}

void MainWindow::on_spinNoiseNew_valueChanged(double arg1)
{
    this->configInput.noiseLevel = arg1;
}

void MainWindow::on_checkNoiseNew_clicked(bool checked)
{
    //this->configInput.autoNoiseLevel = checked;
}

void MainWindow::on_spinLNLNew_valueChanged(int arg1)
{
    //this->configInput.noiseLevelMeasurementsLow = arg1;
}

void MainWindow::on_spinHNLNew_valueChanged(int arg1)
{
    //this->configInput.noiseLevelMeasurementsHigh = arg1;
}
