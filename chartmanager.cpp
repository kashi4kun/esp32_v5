#include "chartmanager.h"

ChartManager::ChartManager(const QString &title, const QString &xLabel, const QString &yLabel, qreal yMin, qreal yMax, QWidget *parent) {
    chart = new QChart();
    series = new QLineSeries();
    chart->addSeries(series);
    series->setName(title);

    axisX = new QValueAxis();
    axisX->setTitleText(xLabel);
    axisX->setRange(0, 300);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis();
    axisY->setTitleText(yLabel);
    axisY->setRange(yMin, yMax);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart, parent);
    chartView->setRenderHint(QPainter::Antialiasing);
}

QChartView* ChartManager::getChartView() {
    return chartView;
}

void ChartManager::updateChart(qint64 elapsedTime, double value) {
    series->append(elapsedTime, value);
    if (elapsedTime >= 300) {
        axisX->setRange(elapsedTime - 300, elapsedTime);
    }
}
