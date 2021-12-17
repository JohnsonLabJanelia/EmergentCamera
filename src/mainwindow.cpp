#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camera.h"
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/bgsegm.hpp>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtCore>
#include <QJsonValue>
#include <QtDebug>
#include <QMessageBox>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QSize>
#include <QProcess>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <emergentcameradef.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentCamera.h>
#include <unistd.h>
#include <sys/time.h>

using namespace cv;
using namespace Emergent;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings = new CameraSettings();
    ui->setupUi(this);

    ui->loadOptions->setToolTip(("Load Movie/Camera Options From JSON File"));
    ui->saveOptions->setToolTip(("Save Movie/Camera Options To JSON File"));
    ui->captureFormatOptions->addItem("Bayer RGB","BayerRGB");
    ui->captureFormatOptions->addItem("YUV 422 Packed","YUV422Packed");
    ui->captureFormatOptions->addItem("RGB 8","RGB8");
    ui->saveFilename->setText("/home/red/Videos");
    ui->compression_h264->setChecked(true);
    ui->recordButton->setText("Record");
    ui->frameRateLineEdit->setText("100");
    ui->exposureLineEdit->setText("5000");
    previewWindow[0] = ui->Camera1_Preview;
    previewWindow[1] = ui->Camera2_Preview;
    previewWindow[2] = ui->Camera3_Preview;
    previewWindow[3] = ui->Camera4_Preview;
    backgroundWindow[0] = ui->Background1;
    backgroundWindow[1] = ui->Background2;
    backgroundWindow[2] = ui->Background3;
    backgroundWindow[3] = ui->Background4;
    trackingWindow[0] = ui->Tracking1;
    trackingWindow[1] = ui->Tracking2;
    trackingWindow[2] = ui->Tracking3;
    trackingWindow[3] = ui->Tracking4;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::ParseOptionsFile() {

}

void MainWindow::on_loadOptions_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                    tr("Load Options File"), "",
                    tr("Movie Options(*.json);;All Files(*)"));
    QFile file;
    file.setFileName(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString val = file.readAll();
    file.close();
    QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject parent = d.object();
    // populate options based off JSON file

    QJsonValue captureFormatValue = parent.value(QString("format"));
    if (!captureFormatValue.isNull()) {
        int index = ui->captureFormatOptions->findData(captureFormatValue);
        ui->captureFormatOptions->setCurrentIndex(index);
    }
    QJsonValue saveLocationValue = parent.value(QString("save_location"));
    if (!saveLocationValue.isNull()) {
        ui->saveFilename->setText(saveLocationValue.toString());
    }

}

void MainWindow::on_saveLocation_clicked()
{
    QString filename = QFileDialog::getExistingDirectory(this, tr("Choose Directory For Recordings"),
                    "/home/rob/Trials",
                    QFileDialog::ShowDirsOnly |
                    QFileDialog::DontResolveSymlinks);
    ui->saveFilename->setText(filename);
}


void MainWindow::on_saveOptions_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                    tr("Save Options File"), "",
                    tr("Movie Options(*.json);;All Files(*)"));
    QFile file;
    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly)) {
         QMessageBox::information(this, tr("Unable to open file handle for writing"),
                 file.errorString());
         return;
    }

    QJsonObject cameraOptions;
    cameraOptions["format"] = QJsonValue::fromVariant(ui->captureFormatOptions->itemData(ui->captureFormatOptions->currentIndex()));
    cameraOptions["save_location"] = QJsonValue::fromVariant(ui->saveFilename->text());
    QString compression("none");
    if (ui->compression_raw->isChecked()) {
        compression = false;
    }
    cameraOptions["compression"] = compression;
    QJsonDocument optionsDocument(cameraOptions);
    file.write(optionsDocument.toJson());
}

void MainWindow::on_frameGrabButton_clicked()
{
    for (int i=0; i<cameras_found; i++) {
        camera[i].GrabBackgroundFrame();
    }

}

void MainWindow::on_recordButton_clicked()
{
    if (!recording) {
        recording = true;
        // switch icon for button to stop
        ui->recordButton->setIcon(QIcon::fromTheme("media-playback-stop"));
        ui->recordButton->setText("Stop Record");

        // switch to nvh265enc for slower but better compression
        QString gstream_options = "appsrc ! videoconvert n-threads=4 ";
        if (ui->compression_h264->isChecked()) {
            gstream_options.append (" ! nvh264enc");
        } else if (ui->compression_h265->isChecked()){
            gstream_options.append (" ! nvh265enc");
        }

        format = QJsonValue::fromVariant(ui->captureFormatOptions->itemData(ui->captureFormatOptions->currentIndex())).toString();
        cout << "FORMAT SELECTED: " << format.toStdString() << endl;

        int fps = ui->frameRateLineEdit->text().toInt();
        QString baseDir(ui->saveFilename->text());
        QString timeFormat = "yyyy-MM-dd_HH:mm";
        QDateTime timestamp = QDateTime::currentDateTime();
        baseDir += QString("/%1").arg(timestamp.toString(timeFormat));
        QDir().mkdir(baseDir);

        for (int i=0; i<cameras_found; i++) {
            if (cameraSelector[i]->isChecked()) {
                camera[i].SetDeviceInfoIndex(i);
                camera[i].setOutputLocation(baseDir);
                camera[i].InitRecord(gstream_options, fps);
                cameraThreads[i] = new QThread;
                camera[i].moveToThread(cameraThreads[i]);
                connect(cameraThreads[i], SIGNAL(started()), &camera[i], SLOT(RecordVideo()));
                connect(cameraThreads[i], SIGNAL(finished()), &camera[i], SLOT(StopCamera()));
                cameraThreads[i]->start();
            }
        }

    } else {
        recording = false;

        // switch icon for button to record
        ui->recordButton->setIcon(QIcon::fromTheme("media-record"));
        ui->recordButton->setText("Record");

        for (int i=0; i<cameras_found; i++) {
            if (cameraSelector[i]->isChecked()) {
                cameraThreads[i]->quit();
                camera[i].StopCamera();
                camera[i].ReleaseWriter();
            }
        }
    }
}

void MainWindow::on_previewButton_clicked()
{
    if (!preview) {
        ui->previewButton->setIcon(QIcon::fromTheme("application-exit"));
        ui->previewButton->setText("Stop Preview");
        preview = true;
        settings->setLatestValues(ui);
        for (int i=0; i<cameras_found; i++) {
            if (cameraSelector[i]->isChecked()) {
                cameraThreads[i] = new QThread;
                camera[i].moveToThread(cameraThreads[i]);
                connect(cameraThreads[i], SIGNAL(started()), &camera[i], SLOT(DisplayPreview()));
               // connect(cameraThreads[i], SIGNAL(finished()), &camera[i], SLOT(StopCamera()));
                cameraThreads[i]->start();
               }
        }
    } else {
        preview = false;

        ui->previewButton->setIcon(QIcon::fromTheme("camera_photo"));
        ui->previewButton->setText("Preview");
        for (int i=0; i<cameras_found; i++) {
            if (cameraSelector[i]->isChecked()) {
                camera[i].StopCamera();
                cameraThreads[i]->quit();
            }
        }
    }

}

void MainWindow::on_frameRateLineEdit_editingFinished()
{
    // set the framerate for cameras


}


bool MainWindow::CheckEmergentCamera(GigEVisionDeviceInfo* deviceInfo) {
    unsigned int listcam_buf_size = MAX_CAMERAS;
    EVT_ListDevices(deviceInfo, &listcam_buf_size, &cameras_found);
    if (cameras_found==0) {
        printf("Enumerate Cameras: \tNo cameras found.\n");
        return false;
    } else {
        cout << cameras_found << " cameras found.\n";
        return true;
    }
}

void MainWindow::on_SetCameraButton_clicked()
{
    struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
    if (!CheckEmergentCamera(deviceInfo)) {
        // take some fatal action
        return;
    }

    QString format = QJsonValue::fromVariant(ui->captureFormatOptions->itemData(ui->captureFormatOptions->currentIndex())).toString();
    int frame_rate = ui->frameRateLineEdit->text().toInt();

    // find out the number of cameras and set up their indices

    cout << "FORMAT SELECTED: " << format.toStdString() << endl;
    for (int i=0; i<cameras_found; i++) {
        camera[i].SetDeviceInfoIndex(i);
        camera[i].setFormat(format);
        camera[i].setFrameRate(frame_rate);
        camera[i].setPreviewFrame(previewWindow[i]);
        camera[i].setBackgroundWindow(backgroundWindow[i]);
        camera[i].setTrackingWindow(trackingWindow[i]);
        camera[i].SetupCamera(deviceInfo);
        int updated_frame_rate = camera[i].getFrameRate();
        ui->frameRateLineEdit->setText(QString::number(updated_frame_rate));
    }

    QVBoxLayout *vbox = new QVBoxLayout;
    for (int i=0; i<cameras_found; i++) {
        QString connectedCameras;
        connectedCameras = camera[i].getName();

       //ui->connectedCameras->setText(connectedCameras);
        cameraSelector[i] = new QCheckBox(connectedCameras);
        cameraSelector[i]->setCheckState(Qt::Checked);
        vbox->addWidget(cameraSelector[i]);
    }
    ui->connectedCamerasGroup->setLayout(vbox);

}

