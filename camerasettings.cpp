#include "camerasettings.h"
#include <QString>
#include <QStringList>
#include <QJsonValue>
#include "ui_mainwindow.h"

CameraSettings::CameraSettings()
{

}

void CameraSettings::setLatestValues(Ui::MainWindow *ui) {
    if (ui->compression_h264->isChecked()) {
        compression = "h264";
    } else {
        compression = "none";
    }
    location = QJsonValue::fromVariant(ui->saveFilename->text()).toString();
}

QString CameraSettings::getCompression() {
    return compression;
}

QString CameraSettings::getLocation() {
    return location;
}

QString CameraSettings::getWidth() {
    return width;
}

QString CameraSettings::getHeight() {
    return height;
}

QString CameraSettings::getFPS() {
    return fps;
}

bool CameraSettings::getIR() {
    return IR;
}

