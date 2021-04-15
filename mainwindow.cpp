#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv2/opencv.hpp>
#include <QTimer>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtDebug>
#include <QMessageBox>
#include <QSize>
#include <QProcess>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentCamera.h>

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

    // combo box items
    ui->cameraSourceOptions->addItem("Device Node 0 (video0)","Device Node 0 (video0)");
    ui->cameraSourceOptions->addItem("Device Node 1 (video1)","Device Node 1 (video1)");
    ui->cameraSourceOptions->addItem("Device Node 2 (video2)","Device Node 2 (video2)");

    ui->resolutionOptions->addItem("640x480","640x480");
    ui->resolutionOptions->addItem("1280x720","1280x720");
    ui->resolutionOptions->addItem("1920x1080","1920x1080");

    ui->fps_options->addItem("30","30");
    ui->fps_options->addItem("60","60");
    ui->fps_options->addItem("120","120");
    ui->fps_options->addItem("240","240");
   // DisplayPreview();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::ParseOptionsFile() {

}

// this method takes the current camera options and generates the appropriate
// gstreamer pipeline command for capturing video
QString MainWindow::GenerateGStreamerPipeline() {
//    return "nvcamerasrc flicker=0 auto-exposure=1 exposure-time=0.0833 color-effect=1 wbmode=0 ! video/x-raw(memory:NVMM), width=(int)" + settings->getWidth() +
  //              ", height=(int)" + settings->getHeight() + ", format=(string)I420, framerate=(fraction)" + settings->getFPS() +
    //            "/1 ! nvvidconv flip-method=0 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";

    return "filesrc location=/home/schauder/Downloads/1.mp4 ! decodebin name=dec ! queue ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

bool MainWindow::CheckEmergentCamera(GigEVisionDeviceInfo* deviceInfo) {
    unsigned int listcam_buf_size = 1;
    unsigned int count;
    EVT_ListDevices(deviceInfo, &listcam_buf_size, &count);
    if (count==0) {
        printf("Enumerate Cameras: \tNo cameras found.\n");
        return false;
    }

    char* EVT_models[] = {"HS", "HT", "HR", "HB", "LR", "LB" };
    int EVT_models_count = sizeof(EVT_models) / sizeof(EVT_models[0]);
    for (int i = 0; i < EVT_models_count; i++) {
        if(strncmp(deviceInfo[0].modelName, EVT_models[i], 2) == 0) {
            return true;
        }
    }
}

void MainWindow::ConfigureEmergentCameraDefaults(CEmergentCamera* camera) {
    unsigned int width_max, height_max, param_val_max;
    const unsigned long enumBufferSize = 1000;
    unsigned long enumBufferSizeReturn = 0;
    char enumBuffer[enumBufferSize];

    //Order is important as param max/mins get updated.
    EVT_CameraGetEnumParamRange(camera, "PixelFormat", enumBuffer, enumBufferSize, &enumBufferSizeReturn);
    char* enumMember = strtok_s(enumBuffer, ",", &next_token);
    EVT_CameraSetEnumParam(camera,      "PixelFormat", enumMember);

    EVT_CameraSetUInt32Param(camera,    "FrameRate", 30);

    EVT_CameraSetUInt32Param(camera,    "OffsetX", 0);
    EVT_CameraSetUInt32Param(camera,    "OffsetY", 0);

    EVT_CameraGetUInt32ParamMax(camera, "Width", &width_max);
    EVT_CameraSetUInt32Param(camera,    "Width", width_max);

    EVT_CameraGetUInt32ParamMax(camera, "Height", &height_max);
    EVT_CameraSetUInt32Param(camera,    "Height", height_max);

    EVT_CameraSetEnumParam(camera,      "AcquisitionMode",        "Continuous");
    EVT_CameraSetUInt32Param(camera,    "AcquisitionFrameCount",  1);
    EVT_CameraSetEnumParam(camera,      "TriggerSelector",        "AcquisitionStart");
    EVT_CameraSetEnumParam(camera,      "TriggerMode",            "Off");
    EVT_CameraSetEnumParam(camera,      "TriggerSource",          "Software");
    EVT_CameraSetEnumParam(camera,      "BufferMode",             "Off");
    EVT_CameraSetUInt32Param(camera,    "BufferNum",              0);

    EVT_CameraGetUInt32ParamMax(camera, "GevSCPSPacketSize", &param_val_max);
    EVT_CameraSetUInt32Param(camera,    "GevSCPSPacketSize", param_val_max);

    EVT_CameraSetUInt32Param(camera,    "Offset", 0);
    EVT_CameraSetUInt32Param(camera, "Exposure", 5000);
    EVT_CameraSetUInt32Param(camera, "Gain", 2000);
    EVT_CameraSetEnumParam(camera, "ColorTemp", "CT_5000K");

    EVT_CameraSetBoolParam(camera,      "LUTEnable", false);
    EVT_CameraSetBoolParam(camera,      "AutoGain", false);
}

void MainWindow::SetupPreview() {
    printf("Starting Preview:");
    struct GigEVisionDeviceInfo deviceInfo[1];


    int ReturnVal = 0;
    int SUCCESS = 0;
    EVT_ERROR err = EVT_SUCCESS;
    unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate;
    unsigned int height_max, width_max;
    unsigned int count, camera_index;
    int cameras_found = 0;

    if (CheckEmergentCamera(deviceInfo)) {
        Emergent::EVT_CameraOpen(&camera, &deviceInfo[0]);

        if (ReturnVal != SUCCESS) {
            printf("Open Camera: \t\tError");
            return;
        }

        ConfigureEmergentCameraDefaults(&camera);
        EVT_CameraGetUInt32ParamMax(&camera, "Height", &height_max);
        EVT_CameraGetUInt32ParamMax(&camera, "Width" , &width_max);
        printf("Resolution: \t\t%d x %d\n", width_max, height_max);
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV422Packed");
        EVT_CameraGetUInt32ParamMax(&camera, "FrameRate", &frame_rate_max);

        frame_rate = frame_rate_max;
        EVT_CameraSetUInt32Param(&camera, "FrameRate", frame_rate);

        printf("FrameRate Set: \t\t%d\n", frame_rate);

        //Prepare host side for streaming.
        ReturnVal = EVT_CameraOpenStream(&camera);
        if (ReturnVal != SUCCESS) {
            printf("Opening Stream from Camera failing: \t\tError");
            return;
        }

        for (int frame_count=0;frame_count<30;frame_count++) {
            evtFrame[frame_count].size_x = width_max;
            evtFrame[frame_count].size_y = height_max;
            evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
            err = EVT_AllocateFrameBuffer(&camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
            if(err) printf("EVT_AllocateFrameBuffer Error!\n");
            err = EVT_CameraQueueFrame(&camera, &evtFrame[frame_count]);
            if(err) printf("EVT_CameraQueueFrame Error!\n");
        }

        //Tell camera to start streaming
        ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
        if (ReturnVal != SUCCESS) {
            printf("EVT_CameraExecuteCommand: Error\n");
            return;
        }
    }
}

void MainWindow::DisplayPreview() {
    int ReturnVal = 0;
    ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
    if (ReturnVal) {
        printf("EVT_CameraQueueFrame Error!\n");
    }
    cv::Mat frame(evtFrameRecv.size_x, evtFrameRecv.size_y, CV_8UC3, evtFrameRecv.imagePtr);
    cv::resize(frame, frame, Size(561, 316), 0, 0, INTER_LINEAR);
    cv::cvtColor(frame, frame, COLOR_BGR2RGB);
    QImage imdisplay((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    ui->VideoPreview->setPixmap(QPixmap::fromImage(imdisplay));

    ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
}

void MainWindow::PipeCameraFrame() {
     cv::Mat frame;
     cap >> frame;
     writer.write(frame);
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

    QJsonValue fpsValue = parent.value(QString("fps"));
    if (!fpsValue.isNull()) {
        int index = ui->fps_options->findData(fpsValue);
        ui->fps_options->setCurrentIndex(index);
    }
    QJsonValue resolutionValue = parent.value(QString("resolution"));
    if (!resolutionValue.isNull()) {
        int index = ui->resolutionOptions->findData(resolutionValue);
        ui->resolutionOptions->setCurrentIndex(index);
    }
    QJsonValue saveLocationValue = parent.value(QString("save_location"));
    if (!saveLocationValue.isNull()) {
        ui->saveFilename->setText(saveLocationValue.toString());
    }
    QJsonValue compressionValue = parent.value(QString("compression"));
    if (!compressionValue.isNull()) {
        if (compressionValue.toString() == "h264") {
            ui->h264_checkbox->setChecked(true);
        }
    }
    QJsonValue sourceValue = parent.value(QString("csi_source"));
    if (!sourceValue.isNull()) {
        int index = ui->cameraSourceOptions->findData(sourceValue);
        ui->cameraSourceOptions->setCurrentIndex(index);
    }
}

void MainWindow::on_saveLocation_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                    tr("Choose where to Save Movie"), "",
                    tr("All Files(*)"));
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
    cameraOptions["fps"] = QJsonValue::fromVariant(ui->fps_options->itemData(ui->fps_options->currentIndex()));
    cameraOptions["resolution"] = QJsonValue::fromVariant(ui->resolutionOptions->itemData(ui->resolutionOptions->currentIndex()));
    cameraOptions["save_location"] = QJsonValue::fromVariant(ui->saveFilename->text());
    QString compression("none");
    if (ui->h264_checkbox->isChecked()) {
        compression = "h264";
    }
    cameraOptions["compression"] = compression;
    cameraOptions["csi_source"] = QJsonValue::fromVariant(ui->cameraSourceOptions->itemData(ui->cameraSourceOptions->currentIndex()));

    QJsonDocument optionsDocument(cameraOptions);
    file.write(optionsDocument.toJson());
}

void MainWindow::on_recordButton_clicked()
{
    if (!recording) {
        recording = true;

        // switch icon for button to stop
        ui->recordButton->setIcon(QIcon::fromTheme("media-playback-stop"));

        // Create OpenCV capture object, ensure it works.
        settings->setLatestValues(ui);
        QString gstreamer = GenerateGStreamerPipeline();
        std::cout << gstreamer.toUtf8().constData();
        cap = cv::VideoCapture(gstreamer.toUtf8().constData(), cv::CAP_GSTREAMER);
        if (!cap.isOpened()) {
            std::cout << "Connection failed";
            return;
        }

        // pipe video capture through writer to file
        QString gstWriter;
        if (settings->getCompression()=="h264") {
            gstWriter = "appsrc !  videoconvert ! omxh264enc ! qtmux ! filesink location=" + settings->getLocation() + " ";
        } else {
            gstWriter = "appsrc !  videoconvert ! qtmux ! filesink location=" + settings->getLocation() + " ";
        }
        std::cout << gstWriter.toUtf8().constData();
        int fourcc;
        if (settings->getCompression()=="h264") {
            fourcc = cv::VideoWriter::fourcc('H','2','6','4');
        } else {
            fourcc = CAP_PROP_FOURCC;
        }
        writer = cv::VideoWriter(gstWriter.toUtf8().constData(), cap.get(fourcc),
                                 settings->getFPS().toFloat(), cv::Size(settings->getWidth().toInt(), settings->getHeight().toInt()));

        timer = new QTimer (this);
        connect(timer, SIGNAL(timeout()), this, SLOT(PipeCameraFrame()));
        timer->start((int)1000.0/settings->getFPS().toDouble());
    } else {
        recording = false;

        // switch icon for button to record
        ui->recordButton->setIcon(QIcon::fromTheme("media-record"));

        timer->stop();
        if (cap.isOpened()) {
            cap.release();
        }
        if (writer.isOpened()) {
            writer.release();
        }
    }
}

void MainWindow::on_previewButton_clicked()
{
    if (!preview) {

        ui->previewButton->setIcon(QIcon::fromTheme("application-exit"));
        preview = true;
        settings->setLatestValues(ui);

       // QString pipeline = "nvcamerasrc flicker=0 auto-exposure=1 exposure-time=0.0833 color-effect=1 wbmode=0 ! video/x-raw(memory:NVMM), width=(int)" + settings->getWidth() +
        //        ", height=(int)" + settings->getHeight() + ", format=(string)I420, framerate=(fraction)" + settings->getFPS() +
        //        "/1 ! nvvidconv flip-method=0 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
        //QString pipeline = "filesrc location=/home/schauder/Downloads/1.mp4 ! decodebin name=dec ! videoconvert ! video/x-raw, format=BGR ! appsink";

        //cap =  cv::VideoCapture(pipeline.toUtf8().constData(), cv::CAP_GSTREAMER);
       // if (!cap.isOpened()) {
       //     std::cout << "Connection failed";
        //    return;
       // }
      //  timer = new QTimer (this);
         //QTimer::singleShot(10000, this, SLOT(DisplayPreview()));
        //connect(timer, SIGNAL(timeout()), this, SLOT(DisplayPreview()));
        //timer->start(30);
        DisplayPreview();
    } else {
        preview = false;

        ui->previewButton->setIcon(QIcon::fromTheme("camera_photo"));
        timer->stop();

        EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

        for(int frame_count=0;frame_count<30;frame_count++) {
                EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
        }

        EVT_CameraCloseStream(&camera);
        EVT_CameraClose(&camera);
        /*if (cap.isOpened()) {
            cap.release();
        }
        if (writer.isOpened()) {
            writer.release();
        }*/
    }

}
