#-------------------------------------------------
#
# Project created by QtCreator 2018-04-20T13:11:22
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Video_Grabber
TEMPLATE = app
INCLUDEPATH += /usr/local/include/opencv4 /opt/EVT/eSDK/include /usr/lib/x86_64-linux-gnu/gstreamer-1.0
LIBS += -L/opt/EVT/eSDK/lib/ -lpthread -lEmergentCamera  -lEmergentGenICam -lEmergentGigEVision -lopencv_core -lopencv_bgsegm -lopencv_imgcodecs -lgstreamer-1.0 -lopencv_imgproc -lopencv_video -lopencv_highgui -lopencv_videoio
SOURCES += main.cpp\
        mainwindow.cpp \
    camerasettings.cpp

HEADERS  += mainwindow.h \
    camerasettings.h

FORMS    += mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/release/ -lopencv_core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/debug/ -lopencv_core
else:unix: LIBS += -L$$PWD/../../../../usr/local/lib/ -lopencv_core -lgstreamer-1.0

INCLUDEPATH += $$PWD/../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../usr/local/include
