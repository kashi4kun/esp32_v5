/********************************************************************************
** Form generated from reading UI file 'ipsettingsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IPSETTINGSDIALOG_H
#define UI_IPSETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_IpSettingsDialog
{
public:
    QLabel *IpSettings;
    QPushButton *saveButton;
    QLineEdit *lineEditIp;

    void setupUi(QDialog *IpSettingsDialog)
    {
        if (IpSettingsDialog->objectName().isEmpty())
            IpSettingsDialog->setObjectName("IpSettingsDialog");
        IpSettingsDialog->resize(400, 300);
        IpSettings = new QLabel(IpSettingsDialog);
        IpSettings->setObjectName("IpSettings");
        IpSettings->setGeometry(QRect(30, 30, 351, 51));
        saveButton = new QPushButton(IpSettingsDialog);
        saveButton->setObjectName("saveButton");
        saveButton->setGeometry(QRect(130, 240, 151, 24));
        lineEditIp = new QLineEdit(IpSettingsDialog);
        lineEditIp->setObjectName("lineEditIp");
        lineEditIp->setGeometry(QRect(40, 111, 331, 31));

        retranslateUi(IpSettingsDialog);

        QMetaObject::connectSlotsByName(IpSettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *IpSettingsDialog)
    {
        IpSettingsDialog->setWindowTitle(QCoreApplication::translate("IpSettingsDialog", "Dialog", nullptr));
        IpSettings->setText(QCoreApplication::translate("IpSettingsDialog", "\320\222\320\262\320\265\320\264\320\270\321\202\320\265 IP \320\260\320\264\321\200\320\265\321\201", nullptr));
        saveButton->setText(QCoreApplication::translate("IpSettingsDialog", "\320\241\320\276\321\205\321\200\320\260\320\275\320\270\321\202\321\214 IP \320\260\320\264\321\200\320\265\321\201", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IpSettingsDialog: public Ui_IpSettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IPSETTINGSDIALOG_H
