#ifndef IPSETTINGSDIALOG_H
#define IPSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class IpSettingsDialog;
}

class IpSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IpSettingsDialog(QWidget *parent = nullptr);
    ~IpSettingsDialog();

    QString getIpAddress() const;

private slots:
    void on_saveButton_clicked();

private:
    Ui::IpSettingsDialog *ui;
};

#endif // IPSETTINGSDIALOG_H
