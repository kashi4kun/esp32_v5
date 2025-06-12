#include "exportdatatofiles.h"
#include "dataProcessor.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDataStream>
#include <QDateTime>
#include <QPointF>
#include <QVector>
#include <QList>

void ExportDataToFiles::exportAllDataToText(const DataProcessor* dp, const QString &baseFilename)
{
    if(!dp) {
        qDebug() << "exportAllDataToText: dataProcessor is null!";
        return;
    }

    // Создаём (или проверяем) папку "Result"
    QDir dir("Result");
    if(!dir.exists()) {
        dir.mkpath(".");
    }

    qint64 startTime = dp->getStartTime();

    // 1) IR
    QString pathIR = dir.absoluteFilePath(baseFilename + "_IR.txt");
    saveVectorTxt(dp->getAllIRData(), startTime, pathIR);

    // 2) Red
    QString pathRed = dir.absoluteFilePath(baseFilename + "_Red.txt");
    saveVectorTxt(dp->getAllRedData(), startTime, pathRed);

    // 3) BPM (временной ряд)
    QString pathBpm = dir.absoluteFilePath(baseFilename + "_BPM.txt");
    saveVectorTxt(dp->getAllBpmData(), startTime, pathBpm);

    // 4) AvgBPM (временной ряд)
    QString pathAvgBpm = dir.absoluteFilePath(baseFilename + "_AvgBPM.txt");
    saveVectorTxt(dp->getAllAvgBpmData(), startTime, pathAvgBpm);

    // 5) Temperature
    QString pathTemp = dir.absoluteFilePath(baseFilename + "_Temp.txt");
    saveVectorTxt(dp->getAllTempData(), startTime, pathTemp);

    // 6) SpO2
    QString pathSpo2 = dir.absoluteFilePath(baseFilename + "_Spo2.txt");
    saveVectorTxt(dp->getAllSpo2Data(), startTime, pathSpo2);

    // 7) Файл с данными по BPM за 1 минуту (среднее, минимум, максимум)
    QString pathBpm1min = dir.absoluteFilePath(baseFilename + "_BPM1min.txt");
    QFile file(pathBpm1min);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "exportAllDataToText: Cannot open file" << pathBpm1min;
    } else {
        QTextStream out(&file);
        // Заголовок файла
        out << "Minute\tAvg BPM\tMin BPM\tMax BPM\n";
        // Получаем накопленные записи за каждую минуту
        const QVector<MinuteBPMData>& records = dp->getMinuteCalculator()->getMinuteBPMRecords();
        for (int i = 0; i < records.size(); ++i) {
            const MinuteBPMData& rec = records[i];
            // Выводим время в формате ЧЧ:ММ и три значения через табуляцию
            out << rec.minuteTimestamp.toString("hh:mm") << "\t"
                << rec.averageBPM << "\t"
                << rec.minBPM << "\t"
                << rec.maxBPM << "\n";
        }
        file.close();
        qDebug() << "Saved BPM 1min TXT:" << pathBpm1min;
    }

    qDebug() << "Text export complete, baseFilename =" << baseFilename;
}

void ExportDataToFiles::exportAllDataToBinary(const DataProcessor* dp, const QString &baseFilename)
{
    if(!dp) {
        qDebug() << "exportAllDataToBinary: dataProcessor is null!";
        return;
    }

    // Папка "Result_Binar"
    QDir dir("Result_Binar");
    if(!dir.exists()) {
        dir.mkpath(".");
    }

    qint64 startTime = dp->getStartTime();

    // 1) IR
    QString pathIR = dir.absoluteFilePath(baseFilename + "_IR.bin");
    saveVectorBin(dp->getAllIRData(), startTime, pathIR);

    // 2) Red
    QString pathRed = dir.absoluteFilePath(baseFilename + "_Red.bin");
    saveVectorBin(dp->getAllRedData(), startTime, pathRed);

    // 3) BPM
    QString pathBpm = dir.absoluteFilePath(baseFilename + "_BPM.bin");
    saveVectorBin(dp->getAllBpmData(), startTime, pathBpm);

    // 4) AvgBPM
    QString pathAvgBpm = dir.absoluteFilePath(baseFilename + "_AvgBPM.bin");
    saveVectorBin(dp->getAllAvgBpmData(), startTime, pathAvgBpm);

    // 5) Temp
    QString pathTemp = dir.absoluteFilePath(baseFilename + "_Temp.bin");
    saveVectorBin(dp->getAllTempData(), startTime, pathTemp);

    // 6) SpO2
    QString pathSpo2 = dir.absoluteFilePath(baseFilename + "_Spo2.bin");
    saveVectorBin(dp->getAllSpo2Data(), startTime, pathSpo2);

    qDebug() << "Binary export complete, baseFilename =" << baseFilename;
}

// --------------------- Приватные методы ---------------------

void ExportDataToFiles::saveVectorTxt(const QVector<QPointF> &data,
                                      qint64 timeStart,
                                      const QString &filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "saveVectorTxt: Cannot open file" << filename;
        return;
    }
    QTextStream out(&file);

    // Для каждой точки (QPointF): x() – это elapsedTime в секундах, y() – значение
    // Восстанавливаем абсолютное время в миллисекундах и выводим в формате
    // "hh:mm:ss <tab> value"
    for(const QPointF &p : data) {
        qint64 absoluteMs = timeStart + static_cast<qint64>(p.x() * 1000);
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(absoluteMs);
        QString timeStr = dt.toString("hh:mm:ss");
        out << timeStr << "\t" << p.y() << "\n";
    }
    file.close();
    qDebug() << "Saved TXT:" << filename;
}

void ExportDataToFiles::saveVectorBin(const QVector<QPointF> &data,
                                      qint64 timeStart,
                                      const QString &filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly)) {
        qDebug() << "saveVectorBin: Cannot open file" << filename;
        return;
    }
    QDataStream out(&file);

    // Формат: (int hour, int min, int sec, int msec, double value)
    // Для каждой точки
    for(const QPointF &p : data) {
        qint64 absoluteMs = timeStart + static_cast<qint64>(p.x() * 1000);
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(absoluteMs);

        int hour   = dt.time().hour();
        int minute = dt.time().minute();
        int second = dt.time().second();
        int msec   = dt.time().msec();
        double val = p.y();

        out << hour << minute << second << msec << val;
    }
    file.close();
    qDebug() << "Saved BIN:" << filename;
}
