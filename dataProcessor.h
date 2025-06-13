#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QLineSeries>
#include <QValueAxis>
#include <QVector>
#include <QDeque>
#include <utility>
#include <QLabel>
#include <QDateTime>
#include <QObject>
#include <cmath>
#include <QPointF>
#include <QList>
#include <QScatterSeries>  // Для отображения пиков

// Структура для хранения данных по BPM за 1 минуту
struct MinuteBPMData {
    QDateTime minuteTimestamp; // Время (начало минуты, локальное)
    double averageBPM;
    double minBPM;
    double maxBPM;
};

class MinuteAverageCalculator : public QObject
{
    Q_OBJECT
public:
    explicit MinuteAverageCalculator(QLabel* avgLabel, QObject* parent = nullptr);
    void addBpmValue(double bpm);
    void updateAverage();
    double getLastAverage() const { return lastAverageBPM; }
    const QVector<MinuteBPMData>& getMinuteBPMRecords() const { return minuteBPMRecords; }

private:
    double calculateAverage(const QVector<double>& values);
    double calculateAverage(const QDeque<std::pair<qint64, double>>& values);
    double calculateMedian(QVector<double> values);
    QVector<double> bpmValues;
    QVector<qint64> bpmTimeStamps;
    double lastAverageBPM = 0.0;
    QLabel* avgMinuteBpmLabel;
    QVector<MinuteBPMData> minuteBPMRecords;
};

class DataProcessor
{
public:
    DataProcessor(QLineSeries* bpmSeries,
                  QLineSeries* avgBpmSeries,
                  QLineSeries* irSeries,
                  QLineSeries* tempSeries,
                  QLineSeries* spo2Series,
                  QLineSeries* spo2PeakSeries,
                  QValueAxis* irAxisX,
                  QValueAxis* bpmAxisX,
                  QValueAxis* avgBpmAxisX,
                  QValueAxis* tempAxisX,
                  QValueAxis* redAxisX,
                  QValueAxis* spo2AxisX,
                  QLabel* avgLabel);

    ~DataProcessor();

    void processValues(qint64 timestamp, double irValue, double redValue, double tempValue);
    bool detectPeakImproved(double irValue, qint64 timestamp);
    double calculateAverage(const QVector<double>& values);
    double detectSpO2(double irValue, double redValue);

    qint64 getStartTime() const { return timeStart; }
    double getElapsedTime() const { return (static_cast<double>(lastReceivedTimestamp - timeStart)) / 1000.0; }

    QLineSeries* getIRSeries() const { return irSeries; }
    QLineSeries* getRedSeries() const { return redSeries; }
    QLineSeries* getBPMSeries() const { return bpmSeries; }
    QLineSeries* getAvgBPMSeries() const { return avgBpmSeries; }
    QLineSeries* getTempSeries() const { return tempSeries; }
    QLineSeries* getSpo2Series() const { return spo2Series; }
    QLineSeries* getSpo2PeakSeries() const { return spo2PeakSeries; }
    // Геттер для серии пиков (QScatterSeries)
    QScatterSeries* getPeakSeries() const { return peakSeries; }

    const QVector<QPointF>& getAllIRData() const { return allIRData; }
    const QVector<QPointF>& getAllRedData() const { return allRedData; }
    const QVector<QPointF>& getAllTempData() const { return allTempData; }
    const QVector<QPointF>& getAllBpmData() const { return allBpmData; }
    const QVector<QPointF>& getAllAvgBpmData() const { return allAvgBpmData; }
    const QVector<QPointF>& getAllSpo2Data() const { return allSpo2Data; }
    const QVector<QPointF>& getAllSpo2PeakData() const { return allSpo2PeakData; }

    const MinuteAverageCalculator* getMinuteCalculator() const { return &minuteCalculator; }

    QValueAxis* getIrAxisX() const { return irAxisX; }
    QValueAxis* getBpmAxisX() const { return bpmAxisX; }
    QValueAxis* getAvgBpmAxisX() const { return avgBpmAxisX; }
    QValueAxis* getTempAxisX() const { return tempAxisX; }
    QValueAxis* getRedAxisX() const { return redAxisX; }
    QValueAxis* getSpo2AxisX() const { return spo2AxisX; }

public:
    enum PeakState { WAITING, RISING };

private:
    QLineSeries* bpmSeries;
    QLineSeries* avgBpmSeries;
    QLineSeries* irSeries;
    QLineSeries* tempSeries;
    QLineSeries* spo2Series;
    QLineSeries* spo2PeakSeries;
    QLineSeries* redSeries;  // Серия для Red

    QVector<QPointF> allIRData;
    QVector<QPointF> allRedData;
    QVector<QPointF> allTempData;
    QVector<QPointF> allBpmData;
    QVector<QPointF> allAvgBpmData;
    QVector<QPointF> allSpo2Data;
    QVector<QPointF> allSpo2PeakData;

    QValueAxis* irAxisX;
    QValueAxis* bpmAxisX;
    QValueAxis* avgBpmAxisX;
    QValueAxis* tempAxisX;
    QValueAxis* redAxisX;
    QValueAxis* spo2AxisX;

    QVector<double> bpmValues;
    qint64 timeStart;
    qint64 lastReceivedTimestamp;
    qint64 lastPeakTime;

    MinuteAverageCalculator minuteCalculator;
    QDeque<std::pair<qint64, double>> redBuffer;
    QDeque<std::pair<qint64, double>> irBuffer;
    QVector<double> intervalIrValues;
    QVector<double> intervalRedValues;
    const int spo2WindowMs;

    // Для детекции пиков по окну:
    QVector<double> peakWindowValues;
    QVector<qint64> peakWindowTimestamps;
    const int windowSize;  // размер окна (например, 5)

    PeakState peakState;
    double previousValue;
    double candidatePeak;
    qint64 candidateTime;
    const double dropThreshold;

    // Серия для отображения пиков (красные точки)
    QScatterSeries* peakSeries;
};

#endif // DATAPROCESSOR_H
