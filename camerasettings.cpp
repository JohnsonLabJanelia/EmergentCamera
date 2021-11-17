#include "camerasettings.h"
#include <QString>
#include <QStringList>
#include <QJsonValue>
#include "ui_mainwindow.h"

CameraSettings::CameraSettings()
{

}

void CameraSettings::setLatestValues(Ui::MainWindow *ui) {
    if (ui->compression_h265->isChecked()) {
        compression = "h264";
    } else {
        compression = "none";
    }
    location = QJsonValue::fromVariant(ui->saveFilename->text()).toString();
    //QString resolution = QJsonValue::fromVariant(ui->resolutionOptions->itemData(ui->resolutionOptions->currentIndex())).toString();
    //QStringList resList = resolution.split("x");
    //width = resList[0];
    //height = resList[1];
    //fps = QJsonValue::fromVariant(ui->fps_options->itemData(ui->fps_options->currentIndex())).toString();
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

