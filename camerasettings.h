#ifndef CAMERASETTINGS_H
#define CAMERASETTINGS_H
#include "ui_mainwindow.h"
#include <QString>

class CameraSettings
{
public:
    CameraSettings();
    void setLatestValues(Ui::MainWindow *ui);
    QString getCompression();
    QString getLocation();
    QString getWidth();
    QString getHeight();
    QString getFPS();
    bool getIR();


private:
    QString compression;
    QString location;
    QString width;
    QString height;
    QString fps;
    bool IR;
};

#endif // CAMERASETTINGS_H
