#include "IpSettingsDialog.h"
#include <QSettings>
#include "ui_IpSettingsDialog.h"

IpSettingsDialog::IpSettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::IpSettingsDialog)
{
    ui->setupUi(this);
    // Задаем заголовок окна (по желанию)
    setWindowTitle("Настройка IP");

    // Загружаем сохранённый IP из QSettings (используем имя организации и приложения)
    QSettings settings("MyCompany", "MyApp");
    QString savedIp = settings.value("ipAddress", "").toString();
    ui->lineEditIp->setText(savedIp);
}

IpSettingsDialog::~IpSettingsDialog()
{
    delete ui;
}

QString IpSettingsDialog::getIpAddress() const
{
    return ui->lineEditIp->text();
}

void IpSettingsDialog::on_saveButton_clicked()
{
    QString ip = ui->lineEditIp->text();
    // Здесь можно добавить проверку корректности IP (например, через QHostAddress::setAddress)
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("ipAddress", ip);
    accept(); // Закрываем диалог с результатом Accepted
}
