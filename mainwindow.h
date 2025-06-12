#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtCharts/QChartView>
#include <QLabel>
#include "dataProcessor.h"
#include "dataReceiver.h"
#include "exportdatatofiles.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    //! Подключение к ESP
    void connectToEsp32();

    //! Диалог настройки IP-адреса
    void onIpSettingsClicked();

    //! Обработка новых данных от датчика (timestamp в мс от ESP32)
    void handleReceivedData(qint64 timestamp, double infraredValue, double redValue, double temperatureValue);

    //! Экспорт данных в текстовые файлы
    void onExportDataText();

    //! Экспорт данных в бинарные файлы
    void onExportDataBinary();
    void checkDataTimeout();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QString currentIpAddress;

    // Виджеты-графики
    QChartView *redChartView;
    QChartView *infraredChartView;
    QChartView *beatsPerMinuteChartView;
    QChartView *averageBpmChartView;
    QChartView *temperatureChartView;
    QChartView *spo2ChartView;

    // Оси X (для графиков, отображаем время в секундах)
    QValueAxis *infraredAxisX;
    QValueAxis *beatsPerMinuteAxisX;
    QValueAxis *averageBpmAxisX;
    QValueAxis *temperatureAxisX;
    QValueAxis *spo2AxisX;

    // Оси Y
    QValueAxis *infraredAxisY;
    QValueAxis *redAxisY;

    // Буферы для автоподстройки осей Y
    QVector<double> lastInfraredValues;
    QVector<double> lastRedValues;

    //! Линия динамического порога
    QLineSeries *thresholdSeriesIR;
    QLineSeries *thresholdSeriesRED;

    //! Метка для среднего BPM за 1 мин.
    QLabel *averageMinuteBpmLabel;

    //! Основная логика обработки
    DataProcessor *dataProcessor;

    //! Приём данных из сокета
    DataReceiver *dataReceiver;
    QDateTime lastDataTime;
    QTimer *dataCheckTimer;

    void setupCharts();
    void setupUiElements();
};

#endif // MAINWINDOW_H
