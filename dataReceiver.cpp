#include "dataReceiver.h"
#include <QDebug>

DataReceiver::DataReceiver(QTcpSocket* socket, QObject* parent)
    : QObject(parent), socket(socket) {}

void DataReceiver::readData() {
    while (socket->canReadLine()) {  // Читаем данные по строкам
        QString line = socket->readLine().trimmed();
        qDebug() << "Received:" << line;

        // Разбиваем строку по запятой
        QStringList parts = line.split(",");
        if (parts.size() == 4) { // Ожидается 4 параметра: timestamp, IR, Red, Temperature
            bool okTimestamp, ok1, ok2, ok3;
            // Первый параметр — временная метка в миллисекундах
            qint64 timestamp = parts[0].trimmed().toLongLong(&okTimestamp);
            double irValue = parts[1].trimmed().toDouble(&ok1);
            double redValue = parts[2].trimmed().toDouble(&ok2);
            double tempValue = parts[3].trimmed().toDouble(&ok3);

            if (okTimestamp && ok1 && ok2 && ok3) {
                emit dataReady(timestamp, irValue, redValue, tempValue);
            } else {
                qDebug() << "Invalid data format: Conversion error.";
            }
        } else {
            qDebug() << "Data format error: Expected 4 parameters, got" << parts.size();
        }
    }
}
