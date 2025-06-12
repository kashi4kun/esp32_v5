#ifndef DATARECEIVER_H
#define DATARECEIVER_H

#include <QTcpSocket>
#include <QObject>

class DataReceiver : public QObject {
    Q_OBJECT
public:
    explicit DataReceiver(QTcpSocket* socket, QObject* parent = nullptr);
    void readData();

signals:
    // Сигнал передает временную метку (в мс) и три значения с датчика
    void dataReady(qint64 timestamp, double irValue, double redValue, double tempValue);

private:
    QTcpSocket* socket;
};

#endif // DATARECEIVER_H
