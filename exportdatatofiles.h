#ifndef EXPORTDATAFILES_H
#define EXPORTDATAFILES_H

#include <QString>
#include <QPointF>
#include <QVector>
#include <QList>
class DataProcessor;

class ExportDataToFiles
{
public:
    // Экспорт всех «полных» данных (IR, Red, BPM, ... ) в текстовые файлы
    static void exportAllDataToText(const DataProcessor* dp, const QString &baseFilename);

    // Экспорт в двоичном (binary) формате
    static void exportAllDataToBinary(const DataProcessor* dp, const QString &baseFilename);

private:
    // Вспомогательные методы для сохранения одного вектора в TXT/BIN
    static void saveVectorTxt(const QVector<QPointF> &data, qint64 startTime, const QString &filename);

    static void saveVectorBin(const QVector<QPointF> &data,
                              qint64 timeStart,
                              const QString &filename);
};

#endif // EXPORTDATAFILES_H
