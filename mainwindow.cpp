#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/bgsegm.hpp>
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
#include <emergentcameradef.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentCamera.h>
#include <unistd.h>
#include <sys/time.h>6383

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

    ui->resolutionOptions->addItem("3208x2200","3208x2200");
    ui->fps_options->addItem("200","200");
    ui->fps_options->addItem("100","100");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::ParseOptionsFile() {

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
  //  EVT_CameraSetEnumParam(camera,      "PixelFormat", enumMember);


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

void MainWindow::SetupCamera() {
    printf("Starting Preview:");
    struct GigEVisionDeviceInfo deviceInfo[1];


    int ReturnVal = 0;
    int SUCCESS = 0;
    EVT_ERROR err = EVT_SUCCESS;

    previewCounter = 0;
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
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "RGB8Packed");
       // EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV422Packed");
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
            evtFrame[frame_count].pixel_type = GVSP_PIX_RGB8;
           // evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
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

    //set up tracker
    //Ptr<Tracker> tracker = TrackerKCF::create();

}

void MainWindow::DisplayPreview() {

    timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    int ReturnVal = 0;
    previewCounter++;
    ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
    if (ReturnVal) {
        printf("EVT_CameraQueueFrame Error!\n");
    }
        previewCounter = 0;

        // uncomment if using a format that needs color space conversion
        //EVT_FrameConvert(&evtFrameRecv, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR);
        cv::Mat frame(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC3, evtFrameRecv.imagePtr);
        cv::resize(frame, frame, Size(561, 316), 0, 0, INTER_LINEAR);
        currFrame = frame;

       QImage imdisplay((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
       ui->VideoPreview->setPixmap(QPixmap::fromImage(imdisplay));

       if (!frameGrabFrame.empty()) {
           cv::Mat frameGrabFrameGray, frameGray;
           cv::Mat diffFrame, cleanFrame;

           cv::cvtColor(frameGrabFrame,frameGrabFrameGray,COLOR_BGR2GRAY );
           cv::cvtColor(frame,frameGray,COLOR_BGR2GRAY );

           cv::absdiff(frameGray,frameGrabFrameGray,diffFrame);
           cv::threshold(diffFrame, diffFrame, 25, 255, THRESH_BINARY);
           cv::dilate(diffFrame, cleanFrame, getStructuringElement(MORPH_RECT, Size(3,3)));

           // this section is a work in progress with getting contours.  Once you have contours, tracking algorithms
           // take over pretty easily
           /*vector<vector<Point> > contours, largestArea;
           vector<Vec4i> hierarchy;
           cv::findContours(cleanFrame,contours, hierarchy,RETR_EXTERNAL, CHAIN_APPROX_NONE);
           sort (contours.begin(), contours.end(), [](const vector<Point>& c1, const vector<Point>& c2){
              return contourArea(c1, false) < contourArea(c2, false);
           });
           if (contours.size()>0) {
               cout << contours[contours.size()-1] << endl;
            }
            // cv::drawContours(cleanFrame,contours,-1, (0,255,0), 3);
           //cout << contours.size() << endl;
*/
           cv::resize(cleanFrame, cleanFrame, Size(261, 161), 0, 0, INTER_LINEAR);

           QImage imdisplay3((uchar*)cleanFrame.data, cleanFrame.cols, cleanFrame.rows, cleanFrame.step, QImage::Format_Grayscale8);
           ui->VideoPreview_3->setPixmap(QPixmap::fromImage(imdisplay3));
       }

     ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
}

void MainWindow::PipeCameraFrame() {
    int ReturnVal = 0;
    ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
    if (ReturnVal) {
        printf("EVT_CameraQueueFrame Error!\n");
        return;
    }
    //EVT_FrameConvert(&evtFrameRecv, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR);
    cv::Mat frame(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC3, evtFrameRecv.imagePtr);

    timeval startTime, endTime;

    // TO DO: Set this up so it will only log for debug mode
    //gettimeofday(&startTime, NULL);
    writer.write(frame);
    //gettimeofday(&endTime, NULL);
    //   float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
    //   cout << "Time of Conversion: " << time_diff << endl;
     ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
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
            ui->h265_checkbox->setChecked(true);
        }
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
    cameraOptions["resolution"] = QJsonValue::fromVariant(ui->resolutionOptions->itemData(ui->resolutionOptions->currentIndex()));
    cameraOptions["save_location"] = QJsonValue::fromVariant(ui->saveFilename->text());
    QString compression("none");
    if (ui->h265_checkbox->isChecked()) {
        compression = true;
    }
    cameraOptions["compression"] = compression;
    QJsonDocument optionsDocument(cameraOptions);
    file.write(optionsDocument.toJson());
}

void MainWindow::on_frameGrabButton_clicked()
{
    frameGrabFrame = currFrame.clone();
    cv::Mat temp;
    cv::resize(frameGrabFrame, temp, Size(261, 161), 0, 0, INTER_LINEAR);

    QImage imdisplay2((uchar*)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    ui->VideoPreview_2->setPixmap(QPixmap::fromImage(imdisplay2));
}

void MainWindow::on_recordButton_clicked()
{
    if (!recording) {
        recording = true;
        // switch icon for button to stop
        ui->recordButton->setIcon(QIcon::fromTheme("media-playback-stop"));

        // switch to nvh265enc for slower but better compression
        QString gstream_options = "appsrc ! videoconvert n-threads=8 ! ";
        if (ui->h265_checkbox->isChecked()) {
            gstream_options.append ("nvh264enc");
        } else {
            gstream_options.append ("nvh265enc");
        }
        gstream_options.append(" ! filesink location=");
        gstream_options += (ui->saveFilename->text());
        writer.open(gstream_options.toUtf8().constData(),CAP_GSTREAMER,0,136, Size(3208,2200));

        cout << writer.isOpened() << endl;
       // if (writer.isOpened()) {
         //   cout << "Writer not initializWed properly";
      //  } else {
            SetupCamera();

            timer = new QTimer (this);
            connect(timer, SIGNAL(timeout()), this, SLOT(PipeCameraFrame()));
            timer->start(7);
       // }
    } else {
        recording = false;

        // switch icon for button to record
        ui->recordButton->setIcon(QIcon::fromTheme("media-record"));

       // if (writer.isOpened()) {
            timer->stop();
            EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

            for(int frame_count=0;frame_count<30;frame_count++) {
                EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
            }

            EVT_CameraCloseStream(&camera);
            EVT_CameraClose(&camera);
        //}
        writer.release();
    }
}

void MainWindow::on_previewButton_clicked()
{
    if (!preview) {

        ui->previewButton->setIcon(QIcon::fromTheme("application-exit"));
        preview = true;
        settings->setLatestValues(ui);
        SetupCamera();

        timer = new QTimer (this);
        connect(timer, SIGNAL(timeout()), this, SLOT(DisplayPreview()));
        timer->start(5);
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
    }

}
