QT       += core gui
QT += core gui network charts
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Включение обработки ошибок, связанных с устаревшими API:
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# Источники
SOURCES += \
    dataProcessor.cpp \
    dataReceiver.cpp \
    exportdatatofiles.cpp \
    ipsettingsdialog.cpp \
    main.cpp \
    mainwindow.cpp

# Заголовочные файлы
HEADERS += \
    dataProcessor.h \
    dataReceiver.h \
    exportdatatofiles.h \
    ipsettingsdialog.h \
    mainwindow.h

# Формы Qt Designer
FORMS += \
    ipsettingsdialog.ui \
    mainwindow.ui

# Правила сборки
unix:!android {
    target.path = /opt/$${TARGET}/bin
    INSTALLS += target
}

qnx: target.path = /tmp/$${TARGET}/bin

# Windows специфические настройки
win32:CONFIG(release, debug|release): DESTDIR = release
win32:CONFIG(debug, debug|release): DESTDIR = debug

