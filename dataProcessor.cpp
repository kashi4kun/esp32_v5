#include "dataProcessor.h"
#include <QDebug>
#include <QDateTime>
#include <cmath>
#include <QPointF>
#include <QList>
#include <algorithm>
#include <QPen>
#include <QBrush>

// ================= MinuteAverageCalculator =================
MinuteAverageCalculator::MinuteAverageCalculator(QLabel* avgLabel, QObject* parent)
    : QObject(parent), avgMinuteBpmLabel(avgLabel)
{}

void MinuteAverageCalculator::addBpmValue(double bpm) {
    bpmValues.append(bpm);
    bpmTimeStamps.append(QDateTime::currentSecsSinceEpoch());
    qDebug() << "Added BPM:" << bpm << "Total:" << bpmValues.size();
}

double MinuteAverageCalculator::calculateAverage(const QVector<double>& values) {
    if (values.isEmpty())
        return 0.0;
    double sum = 0.0;
    for (double v : values)
        sum += v;
    return sum / values.size();
}

double MinuteAverageCalculator::calculateMedian(QVector<double> values) {
    if (values.isEmpty())
        return 0.0;
    std::sort(values.begin(), values.end());
    int n = values.size();
    if (n % 2 == 0)
        return (values[n / 2 - 1] + values[n / 2]) / 2.0;
    else
        return values[n / 2];
}

void MinuteAverageCalculator::updateAverage() {
    qDebug() << "Updating average at:" << QDateTime::currentDateTime().toString("hh:mm:ss");
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    // Удаляем значения старше 1 минуты
    while (!bpmTimeStamps.isEmpty() && currentTime - bpmTimeStamps.first() > 60) {
        bpmTimeStamps.pop_front();
        bpmValues.pop_front();
    }

    if (bpmValues.isEmpty()) {
        avgMinuteBpmLabel->setText("Avg BPM (1 min): --");
        return;
    }

    QVector<double> filtered = bpmValues;
    double median = calculateMedian(filtered);
    double lowerBound = median * 0.8;
    double upperBound = (median > 120) ? median * 1.3 : median * 1.2;

    filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
                                  [lowerBound, upperBound](double bpm) {
                                      return bpm < lowerBound || bpm > upperBound;
                                  }),
                   filtered.end());

    if (!filtered.isEmpty()) {
        double avgBpm = calculateAverage(filtered);
        double minBpm = *std::min_element(filtered.begin(), filtered.end());
        double maxBpm = *std::max_element(filtered.begin(), filtered.end());
        lastAverageBPM = avgBpm;
        qDebug() << "Minute stats: average =" << avgBpm << "min =" << minBpm << "max =" << maxBpm;

        QDateTime currentDt = QDateTime::currentDateTime();
        QTime t = currentDt.time();
        t = QTime(t.hour(), t.minute(), 0);
        currentDt.setTime(t);

        MinuteBPMData record;
        record.minuteTimestamp = currentDt;
        record.averageBPM = avgBpm;
        record.minBPM = minBpm;
        record.maxBPM = maxBpm;
        minuteBPMRecords.append(record);

        avgMinuteBpmLabel->setText("Avg BPM (1 min): " + QString::number(avgBpm, 'f', 2));
    } else {
        avgMinuteBpmLabel->setText("Avg BPM (1 min): --");
    }
}

// ================= DataProcessor =================
DataProcessor::DataProcessor(QLineSeries* bpmSeries,
                             QLineSeries* avgBpmSeries,
                             QLineSeries* irSeries,
                             QLineSeries* tempSeries,
                             QLineSeries* spo2Series,
                             QValueAxis* irAxisX,
                             QValueAxis* bpmAxisX,
                             QValueAxis* avgBpmAxisX,
                             QValueAxis* tempAxisX,
                             QValueAxis* redAxisX,
                             QValueAxis* spo2AxisX,
                             QLabel* avgLabel)
    : bpmSeries(bpmSeries),
    avgBpmSeries(avgBpmSeries),
    irSeries(irSeries),
    tempSeries(tempSeries),
    spo2Series(spo2Series),
    irAxisX(irAxisX),
    bpmAxisX(bpmAxisX),
    avgBpmAxisX(avgBpmAxisX),
    tempAxisX(tempAxisX),
    redAxisX(redAxisX),
    spo2AxisX(spo2AxisX),
    minuteCalculator(avgLabel, nullptr),
    timeStart(0),
    lastReceivedTimestamp(0),
    lastPeakTime(0),
    redSeries(new QLineSeries()),
    peakState(WAITING),
    previousValue(0.0),
    candidatePeak(0.0),
    candidateTime(0),
    dropThreshold(0.001), // Порог падения 0.1%
    windowSize(5)  // Размер окна для детекции пика
{
    qDebug() << "DataProcessor constructor completed";

    // Создаем серию для пиков и настраиваем её внешний вид:
    peakSeries = new QScatterSeries();
    peakSeries->setName("Peak Points");
    QPen pen(Qt::red);
    pen.setWidth(2);
    peakSeries->setPen(pen);
    peakSeries->setBrush(QBrush(Qt::red));
    peakSeries->setMarkerSize(10.0);
}

double DataProcessor::calculateAverage(const QVector<double>& values) {
    if (values.isEmpty())
        return 0;
    double sum = 0;
    for (double v : values)
        sum += v;
    return sum / values.size();
}

double DataProcessor::detectSpO2(double irValue, double redValue) {
    // Логика расчёта SpO₂ может быть реализована здесь
    return 0.0;
}

// Сохраняем старую функцию, если понадобится (но новый алгоритм в processValues используется для пиков)
bool DataProcessor::detectPeakImproved(double irValue, qint64 timestamp) {
    bool peakDetected = false;
    if (peakState == WAITING) {
        if (irValue > previousValue) {
            peakState = RISING;
            candidatePeak = irValue;
            candidateTime = timestamp;
        }
    } else if (peakState == RISING) {
        if (irValue > candidatePeak) {
            candidatePeak = irValue;
            candidateTime = timestamp;
        } else if (irValue < candidatePeak * (1 - dropThreshold)) {
            peakDetected = true;
            peakState = WAITING;
        }
    }
    previousValue = irValue;
    return peakDetected;
}

void DataProcessor::processValues(qint64 timestamp, double infraredValue, double redValue, double temperatureValue) {
    // Если timeStart еще не установлен, сохраняем первую временную метку
    if (timeStart == 0)
        timeStart = timestamp;
    // Вычисляем время относительно первого значения (начало = 0)
    double currentTimeSec = static_cast<double>(timestamp - timeStart) / 1000.0;
    lastReceivedTimestamp = timestamp;
    qDebug() << "Processing IR=" << infraredValue
             << ", Red=" << redValue
             << ", Temp=" << temperatureValue
             << ", currentTimeSec=" << currentTimeSec;

    // Вычисляем DC-компоненты для буферов
    double infraredDC = calculateAverage(irBuffer);
    double redDC = calculateAverage(redBuffer);
    double infraredAC = infraredValue - infraredDC;
    double redAC = redValue - redDC;

    // Расчёт SpO₂, если данные валидны
    if (infraredDC != 0 && redDC != 0 && infraredAC > 0 && redAC > 0) {
        double ratioR = (redAC / redDC) / (infraredAC / infraredDC);
        int spo2 = static_cast<int>(110 - 25.0 * ratioR);
        spo2 = qBound(80, spo2, 100);
        qDebug() << "Calculated SpO₂=" << spo2;
        spo2Series->append(currentTimeSec, spo2);
        allSpo2Data.append(QPointF(currentTimeSec, spo2));
    }

    // Сохраняем данные для экспорта и добавляем их в графики
    allIRData.append(QPointF(currentTimeSec, infraredValue));
    allRedData.append(QPointF(currentTimeSec, redValue));
    allTempData.append(QPointF(currentTimeSec, temperatureValue));

    irSeries->append(currentTimeSec, infraredValue);
    redSeries->append(currentTimeSec, redValue);
    tempSeries->append(currentTimeSec, temperatureValue);

    // Обновляем диапазон оси X для отображения последних 20 секунд
    if (currentTimeSec >= 20.0) {
        irAxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
        bpmAxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
        avgBpmAxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
        tempAxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
        redAxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
        spo2AxisX->setRange(currentTimeSec - 20.0, currentTimeSec);
    } else {
        irAxisX->setRange(0.0, 20.0);
        bpmAxisX->setRange(0.0, 20.0);
        avgBpmAxisX->setRange(0.0, 20.0);
        tempAxisX->setRange(0.0, 20.0);
        redAxisX->setRange(0.0, 20.0);
        spo2AxisX->setRange(0.0, 20.0);
    }

    // --- Алгоритм детекции пиков с использованием окна ---
    // Добавляем текущую точку в окно
    peakWindowValues.push_back(infraredValue);
    peakWindowTimestamps.push_back(timestamp);

    // Если окно превышает размер, удаляем самую старую точку
    if (peakWindowValues.size() > windowSize) {
        peakWindowValues.pop_front();
        peakWindowTimestamps.pop_front();
    }

    // Если в окне ровно windowSize точек, проверяем центральную точку
    if (peakWindowValues.size() == windowSize) {
        int mid = windowSize / 2; // для windowSize=5, mid=2
        bool isPeak = true;
        for (int i = 0; i < windowSize; i++) {
            if (i == mid)
                continue;
            if (peakWindowValues[mid] <= peakWindowValues[i]) {
                isPeak = false;
                break;
            }
        }
        if (isPeak) {
            qint64 detectedPeakTime = peakWindowTimestamps[mid];
            // Вводим рефрактерный период: если предыдущий пик отсутствует или разница > 300 мс
            if (lastPeakTime == 0 || (detectedPeakTime - lastPeakTime) > 300) {
                double peakTimeSec = static_cast<double>(detectedPeakTime - timeStart) / 1000.0;
                // Добавляем красную точку в серию пиков
                peakSeries->append(peakTimeSec, peakWindowValues[mid]);
                // Если имеется предыдущий пик, можно вычислить интервал для BPM
                if (lastPeakTime != 0) {
                    int deltaMs = static_cast<int>(detectedPeakTime - lastPeakTime);
                    qDebug() << "Peak interval (ms):" << deltaMs;
                    if (deltaMs > 500 && deltaMs < 1333) {
                        double bpm = 60000.0 / deltaMs;
                        qDebug() << "Calculated BPM:" << bpm;
                        bpmValues.push_back(bpm);
                        if (bpmValues.size() > 3)
                            bpmValues.pop_front();
                        double avgBpmLocal = calculateAverage(bpmValues);
                        bpmSeries->append(peakTimeSec, bpm);
                        avgBpmSeries->append(peakTimeSec, avgBpmLocal);
                        allBpmData.append(QPointF(peakTimeSec, bpm));
                        allAvgBpmData.append(QPointF(peakTimeSec, avgBpmLocal));
                        minuteCalculator.addBpmValue(bpm);
                    }
                }
                lastPeakTime = detectedPeakTime;
            }
        }
    }
    // --- Конец алгоритма детекции пиков ---

    // Обновляем буферы для SpO₂
    irBuffer.push_back(infraredValue);
    redBuffer.push_back(redValue);
    if (irBuffer.size() > 100) irBuffer.pop_front();
    if (redBuffer.size() > 100) redBuffer.pop_front();
}
