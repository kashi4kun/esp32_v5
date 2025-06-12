#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGridLayout>
#include <QPushButton>
#include <QTimer>
#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include "ipsettingsdialog.h"
#include "exportDataToFiles.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QFont font;
    font.setPointSize(14);

    QApplication::setFont(font);  // Это изменит шрифт для всех виджетов приложения
    // Создаем серии для динамических порогов
    thresholdSeriesIR = new QLineSeries(this);
    thresholdSeriesIR->setName("Dynamic Threshold");
    thresholdSeriesRED = new QLineSeries(this);
    thresholdSeriesRED->setName("Dynamic Threshold");

    // Метка для среднего BPM за 1 минуту
    averageMinuteBpmLabel = new QLabel("Avg BPM (1 min): --", this);

    // Серия для SpO₂
    QLineSeries *bloodOxygenSaturationSeries = new QLineSeries();

    // Создаем оси для графиков (отображение в секундах)
    QValueAxis *axisIrX   = new QValueAxis();
    QValueAxis *axisBpmX  = new QValueAxis();
    QValueAxis *axisAvgX  = new QValueAxis();
    QValueAxis *axisTempX = new QValueAxis();
    QValueAxis *axisRedX  = new QValueAxis();
    QValueAxis *axisSpo2X = new QValueAxis();

    // Инициализация DataProcessor – внутри он работает с миллисекундами,
    // а для графиков конвертирует время в секунды
    dataProcessor = new DataProcessor(
        new QLineSeries(), // BPM series
        new QLineSeries(), // Avg BPM series
        new QLineSeries(), // IR series
        new QLineSeries(), // Temperature series
        bloodOxygenSaturationSeries, // SpO₂ series
        axisIrX, axisBpmX, axisAvgX, axisTempX, axisRedX, axisSpo2X,
        averageMinuteBpmLabel
        );

    dataCheckTimer = new QTimer(this);
    dataCheckTimer->setInterval(10000); // Проверка каждые 10 с
    connect(dataCheckTimer, &QTimer::timeout,
            this, &MainWindow::checkDataTimeout);
    lastDataTime = QDateTime::currentDateTime();
    dataCheckTimer->start();

    // Создаем QTcpSocket и подключаемся к ESP32
    socket = new QTcpSocket(this);
    socket->setReadBufferSize(0);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onSocketDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    // Инициализируем графики
    redChartView         = new QChartView(this);
    infraredChartView    = new QChartView(this);
    beatsPerMinuteChartView = new QChartView(this);
    averageBpmChartView  = new QChartView(this);
    temperatureChartView = new QChartView(this);
    spo2ChartView        = new QChartView(this);

    {
        QChart *redChart = new QChart();
        redChart->legend()->setVisible(false);
        redChart->setTitle("Red Data");
        QValueAxis* axisRedXPtr = dataProcessor->getRedAxisX();
        axisRedXPtr->setTitleText("Time (s)");
        axisRedXPtr->setRange(0, 300);
        redChart->addAxis(axisRedXPtr, Qt::AlignBottom);

        redAxisY = new QValueAxis();
        redAxisY->setTitleText("Red Value");
        redAxisY->setRange(20000, 30000);
        redChart->addAxis(redAxisY, Qt::AlignLeft);

        redChart->addSeries(dataProcessor->getRedSeries());
        dataProcessor->getRedSeries()->attachAxis(axisRedXPtr);
        dataProcessor->getRedSeries()->attachAxis(redAxisY);

        redChart->addSeries(thresholdSeriesRED);
        thresholdSeriesRED->attachAxis(axisRedXPtr);
        thresholdSeriesRED->attachAxis(redAxisY);

        redChartView->setChart(redChart);
    }

    {
        QChart *irChart = new QChart();
        irChart->legend()->setVisible(false);
        irChart->setTitle("Infrared (IR) Data");
        QValueAxis *axisIrXPtr = dataProcessor->getIrAxisX();
        axisIrXPtr->setTitleText("Time (s)");
        axisIrXPtr->setRange(0, 300);
        irChart->addAxis(axisIrXPtr, Qt::AlignBottom);

        infraredAxisY = new QValueAxis();
        infraredAxisY->setTitleText("IR Value");
        infraredAxisY->setRange(95000, 110000);
        irChart->addAxis(infraredAxisY, Qt::AlignLeft);

        // Основная серия IR
        irChart->addSeries(dataProcessor->getIRSeries());
        dataProcessor->getIRSeries()->attachAxis(axisIrXPtr);
        dataProcessor->getIRSeries()->attachAxis(infraredAxisY);

        // Пороговая серия (если используется)
        irChart->addSeries(thresholdSeriesIR);
        thresholdSeriesIR->attachAxis(axisIrXPtr);
        thresholdSeriesIR->attachAxis(infraredAxisY);

        // Добавляем серию пиков (красные точки)
        irChart->addSeries(dataProcessor->getPeakSeries());
        dataProcessor->getPeakSeries()->attachAxis(axisIrXPtr);
        dataProcessor->getPeakSeries()->attachAxis(infraredAxisY);

        infraredChartView->setChart(irChart);
    }


    {
        QChart *bpmChart = new QChart();
        bpmChart->legend()->setVisible(false);
        bpmChart->setTitle("Beats Per Minute");
        QValueAxis *axisBpmXPtr = dataProcessor->getBpmAxisX();
        axisBpmXPtr->setTitleText("Time (s)");
        axisBpmXPtr->setRange(0, 300);
        bpmChart->addAxis(axisBpmXPtr, Qt::AlignBottom);

        QValueAxis *bpmAxisY = new QValueAxis();
        bpmAxisY->setTitleText("BPM Value");
        bpmAxisY->setRange(50, 140);
        bpmChart->addAxis(bpmAxisY, Qt::AlignLeft);

        bpmChart->addSeries(dataProcessor->getBPMSeries());
        dataProcessor->getBPMSeries()->attachAxis(axisBpmXPtr);
        dataProcessor->getBPMSeries()->attachAxis(bpmAxisY);

        beatsPerMinuteChartView->setChart(bpmChart);
    }

    {
        QChart *avgBpmChart = new QChart();
        avgBpmChart->legend()->setVisible(false);
        avgBpmChart->setTitle("Average BPM");
        QValueAxis *axisAvgBpmXPtr = dataProcessor->getAvgBpmAxisX();
        axisAvgBpmXPtr->setTitleText("Time (s)");
        axisAvgBpmXPtr->setRange(0, 300);
        avgBpmChart->addAxis(axisAvgBpmXPtr, Qt::AlignBottom);

        QValueAxis *avgBpmAxisY = new QValueAxis();
        avgBpmAxisY->setTitleText("Avg BPM");
        avgBpmAxisY->setRange(50, 140);
        avgBpmChart->addAxis(avgBpmAxisY, Qt::AlignLeft);

        avgBpmChart->addSeries(dataProcessor->getAvgBPMSeries());
        dataProcessor->getAvgBPMSeries()->attachAxis(axisAvgBpmXPtr);
        dataProcessor->getAvgBPMSeries()->attachAxis(avgBpmAxisY);

        averageBpmChartView->setChart(avgBpmChart);
    }

    {
        QChart *tempChart = new QChart();
        tempChart->legend()->setVisible(false);
        tempChart->setTitle("Temperature Data");
        QValueAxis *axisTempXPtr = dataProcessor->getTempAxisX();
        axisTempXPtr->setTitleText("Time (s)");
        axisTempXPtr->setRange(0, 300);
        tempChart->addAxis(axisTempXPtr, Qt::AlignBottom);

        QValueAxis *tempAxisY = new QValueAxis();
        tempAxisY->setTitleText("Temp. (°C)");
        tempAxisY->setRange(25, 38);
        tempChart->addAxis(tempAxisY, Qt::AlignLeft);

        tempChart->addSeries(dataProcessor->getTempSeries());
        dataProcessor->getTempSeries()->attachAxis(axisTempXPtr);
        dataProcessor->getTempSeries()->attachAxis(tempAxisY);

        temperatureChartView->setChart(tempChart);
    }

    {
        QChart *spo2Chart = new QChart();
        spo2Chart->legend()->setVisible(false);
        spo2Chart->setTitle("SpO₂ Data");
        QValueAxis *axisSpo2XPtr = dataProcessor->getSpo2AxisX();
        axisSpo2XPtr->setTitleText("Time (s)");
        axisSpo2XPtr->setRange(0, 300);
        spo2Chart->addAxis(axisSpo2XPtr, Qt::AlignBottom);

        QValueAxis *spo2AxisY = new QValueAxis();
        spo2AxisY->setTitleText("SpO₂ (%)");
        spo2AxisY->setRange(90, 105);
        spo2Chart->addAxis(spo2AxisY, Qt::AlignLeft);

        spo2Chart->addSeries(dataProcessor->getSpo2Series());
        dataProcessor->getSpo2Series()->attachAxis(axisSpo2XPtr);
        dataProcessor->getSpo2Series()->attachAxis(spo2AxisY);

        spo2ChartView->setChart(spo2Chart);
    }

    // Кнопки экспорта и настройки IP
    QPushButton *exportDataTextButton = new QPushButton("Export Data (Text)", this);
    connect(exportDataTextButton, &QPushButton::clicked,
            this, &MainWindow::onExportDataText);

    QPushButton *exportDataBinButton = new QPushButton("Export Data (Binary)", this);
    connect(exportDataBinButton, &QPushButton::clicked,
            this, &MainWindow::onExportDataBinary);

    QPushButton *ipSettingsButton = new QPushButton("Настройка IP", this);
    connect(ipSettingsButton, &QPushButton::clicked,
            this, &MainWindow::onIpSettingsClicked);

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(redChartView,            0, 0);
    layout->addWidget(infraredChartView,       0, 1);
    layout->addWidget(beatsPerMinuteChartView, 1, 0);
    layout->addWidget(averageBpmChartView,     1, 1);
    layout->addWidget(temperatureChartView,    2, 0);
    layout->addWidget(spo2ChartView,           2, 1);
    layout->addWidget(exportDataTextButton,    4, 0, 1, 2);
    layout->addWidget(exportDataBinButton,     5, 0, 1, 2);
    layout->addWidget(ipSettingsButton,        6, 0, 1, 2);

    QWidget *centralW = new QWidget();
    centralW->setLayout(layout);
    setCentralWidget(centralW);

    // Создаем DataReceiver и соединяем его сигнал с нашим слотом
    dataReceiver = new DataReceiver(socket, this);
    connect(socket, &QTcpSocket::readyRead, dataReceiver, &DataReceiver::readData);
    // Обновленный сигнал dataReady теперь передает timestamp (в мс) + 3 значения
    connect(dataReceiver, &DataReceiver::dataReady,
            this, &MainWindow::handleReceivedData);

    connectToEsp32();

    // Таймер для обновления среднего BPM за минуту
    QTimer *minuteTimer = new QTimer(this);
    connect(minuteTimer, &QTimer::timeout,
            dataProcessor->getMinuteCalculator(), &MinuteAverageCalculator::updateAverage);
    minuteTimer->start(60000);
    qDebug() << "Minute-average BPM timer started, interval =" << minuteTimer->interval();
}

MainWindow::~MainWindow() {
    delete dataProcessor;
    delete ui;
}

//------------------------------------------------------------------------------
// Попытка подключения к ESP32
//------------------------------------------------------------------------------
void MainWindow::connectToEsp32() {
    QSettings settings("MyCompany", "MyApp");
    currentIpAddress = settings.value("ipAddress", "192.168.31.222").toString();
    qDebug() << "Attempting to connect to" << currentIpAddress << "on port 80...";
    socket->abort();
    socket->connectToHost(currentIpAddress, 80);
    if (!socket->waitForConnected(5000)) {
        qDebug() << "Not connected. Will retry in 5s...";
        QTimer::singleShot(5000, this, &MainWindow::connectToEsp32);
    } else {
        qDebug() << "Successfully connected to" << currentIpAddress;
    }
}

//------------------------------------------------------------------------------
// Настройка IP-адреса
//------------------------------------------------------------------------------
void MainWindow::onIpSettingsClicked() {
    IpSettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString newIp = dlg.getIpAddress();
        qDebug() << "New IP address saved:" << newIp;
        currentIpAddress = newIp;
        QSettings settings("MyCompany", "MyApp");
        settings.setValue("ipAddress", newIp);
        socket->disconnectFromHost();
        socket->connectToHost(currentIpAddress, 80);
    }
}

//------------------------------------------------------------------------------
// Приходят новые данные из датчика
//------------------------------------------------------------------------------
// Слот теперь принимает timestamp (в мс) от ESP32, а на графиках используется время в секундах
void MainWindow::handleReceivedData(qint64 timestamp, double infraredValue,
                                    double redValue, double temperatureValue)
{
    lastDataTime = QDateTime::currentDateTime();
    // Передаем данные (timestamp в мс) в DataProcessor
    dataProcessor->processValues(timestamp, infraredValue, redValue, temperatureValue);

    // Обновляем буферы для автоподстройки осей Y
    lastInfraredValues.append(infraredValue);
    if (lastInfraredValues.size() > 10)
        lastInfraredValues.pop_front();
    if (lastInfraredValues.size() == 10) {
        double sumIr = 0.0;
        for (double v : lastInfraredValues)
            sumIr += v;
        double avgIr = sumIr / lastInfraredValues.size();
        infraredAxisY->setRange(avgIr - 500, avgIr + 500);
    }
    lastRedValues.append(redValue);
    if (lastRedValues.size() > 10)
        lastRedValues.removeFirst();
    if (lastRedValues.size() == 10) {
        double sumRed = 0.0;
        for (double v : lastRedValues)
            sumRed += v;
        double avgRed = sumRed / lastRedValues.size();
        redAxisY->setRange(avgRed - 200, avgRed + 200);
    }

}

void MainWindow::checkDataTimeout() {
    qint64 secsSinceLastData = lastDataTime.secsTo(QDateTime::currentDateTime());
    if (secsSinceLastData > 10) {
        qDebug() << "No data from ESP32 for" << secsSinceLastData
                 << "seconds. Reconnecting...";
        socket->disconnectFromHost();
        socket->abort();
        connectToEsp32();
    }
}

void MainWindow::onSocketDisconnected() {
    qDebug() << "Socket disconnected. Reconnect in 5 seconds...";
    QTimer::singleShot(5000, this, &MainWindow::connectToEsp32);
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError) {
    qDebug() << "Socket error:" << socketError
             << "description=" << socket->errorString();
    QTimer::singleShot(3000, this, &MainWindow::connectToEsp32);
}

void MainWindow::onExportDataText() {
    QString baseFilename = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    ExportDataToFiles::exportAllDataToText(dataProcessor, baseFilename);
}

void MainWindow::onExportDataBinary() {
    QString baseFilename = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    ExportDataToFiles::exportAllDataToBinary(dataProcessor, baseFilename);
}
