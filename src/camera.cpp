#include "camera.h"
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
#include <QMutex>
#include <QMutexLocker>
#include <QSize>
#include <QProcess>
#include <QLabel>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <emergentcameradef.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentCamera.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <QDateTime>

Camera::Camera(QObject * parent) : QObject (parent)
{
    mutex = new QMutex();
}

int Camera::GetDeviceInfoIndex() {
    return deviceInfoIndex;
}

void Camera::SetDeviceInfoIndex(int index) {
    deviceInfoIndex = index;
}

QString Camera::getFormat() const
{
    return format;
}

void Camera::setFormat(const QString &value)
{
    format = value;
}

void Camera::setFrameRate(int value)
{
    frame_rate = value;
}

int Camera::getFrameRate()
{
    return frame_rate;
}

void Camera::cleanUpCamera()
{

}

CEmergentCamera* Camera::getEmergentCameraPtr() {
    return &camera;
}

void Camera::ConfigureEmergentCameraDefaults(CEmergentCamera* camera) {
    const unsigned long enumBufferSize = 1000;
    unsigned int param_val_max;
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

void Camera::ReleaseFrame(int frameIndex) {
    EVT_ReleaseFrameBuffer(&camera, &evtFrame[frameIndex]);
}

void Camera::SetupCamera(GigEVisionDeviceInfo* deviceInfo) {
    int ReturnVal = 0;
    int SUCCESS = 0;
    EVT_ERROR err = EVT_SUCCESS;

    Emergent::EVT_CameraOpen(&camera, &deviceInfo[deviceInfoIndex]);
    name = deviceInfo[deviceInfoIndex].serialNumber;
    name.prepend("Serial Number: ");
    cout << name.toUtf8().constData() << endl;

    if (ReturnVal != SUCCESS) {
        printf("Open Camera: \t\tError");
        return;
    }

    ConfigureEmergentCameraDefaults(&camera);
    EVT_CameraGetUInt32ParamMax(&camera, "Height", &height_max);
    EVT_CameraGetUInt32ParamMax(&camera, "Width" , &width_max);
    printf("Resolution: \t\t%d x %d\n", width_max, height_max);
    if (format == "RGB8") {
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "RGB8Packed");
    } else if (format == "BayerRGB") {
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerRG8");
    } else {
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV422Packed");
    }

    EVT_CameraGetUInt32ParamMax(&camera, "FrameRate", &frame_rate_max);

    if(frame_rate > frame_rate_max){
        setFrameRate(frame_rate_max);
    }

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
        if (format == "RGB8") {
            evtFrame[frame_count].pixel_type = GVSP_PIX_RGB8;
        } else if (format == "BayerRGB") {
            evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG8;
        } else {
            evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
        }
        err = EVT_AllocateFrameBuffer(&camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
        if(err) printf("EVT_AllocateFrameBuffer Error!\n");
        err = EVT_CameraQueueFrame(&camera, &evtFrame[frame_count]);
        if(err) printf("EVT_CameraQueueFrame Error!\n");
    }



}

void Camera::InitRecord(QString gstream_template, int fps)
{
    int ReturnVal = 0;
    int SUCCESS = 0;
    //Tell camera to start streaming
    ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
    if (ReturnVal != SUCCESS) {
        printf("EVT_CameraExecuteCommand: Error\n");
        return;
    }
    loop_stopped = false;

    QString recordOptions(gstream_template);
    recordOptions.append(" ! filesink location=");
    recordOptions += QString("%1/recordingOutput%2.mp4").arg(outputLocation).arg(deviceInfoIndex);

    cout << recordOptions.toUtf8().constData() << endl;
    writer.open(recordOptions.toUtf8().constData(),CAP_GSTREAMER,0,fps, Size(3208,2200));
}

void Camera::ReleaseWriter()
{
    writer.release();
}

void Camera::DisplayPreview() {
    int ReturnVal = 0;
    int SUCCESS = 0;
    //Tell camera to start streaming
    ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
    if (ReturnVal != SUCCESS) {
        printf("EVT_CameraExecuteCommand: Error\n");
        return;
    }
    loop_stopped = false;

    while (true) {
        // this part exits loop if bool is set to true
        //QMutexLocker locker(mutex);
        if (loop_stopped) {
            break;
        }

        //Tell camera to start streaming
        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        int ReturnVal = 0;
        ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
        if (ReturnVal !=0) {
            cout << "EVT_CameraQueueFrame Error! Error Code:" << ReturnVal << endl;
        }

        cv::Mat frame;
        if (format == "RGB8") {
            frame = cv::Mat(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC3, evtFrameRecv.imagePtr);
        } else if (format == "BayerRGB") {
            cv::Mat frameConvert(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC1, evtFrameRecv.imagePtr);
            cv::cvtColor(frameConvert,frame,COLOR_BayerRG2RGB );
        } else {
            cv::Mat frameConvert(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC2, evtFrameRecv.imagePtr);
            cv::cvtColor(frameConvert,frame,COLOR_YUV2BGR_Y422 );
        }
        cv::resize(frame, frame, Size(561, 316), 0, 0, INTER_LINEAR);
        currFrame = frame;

        QImage imdisplay((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        previewFrame->setPixmap(QPixmap::fromImage(imdisplay));
        ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
        gettimeofday(&endTime, NULL);
        float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
        // cout << "Device :" << deviceInfoIndex << "Processing Time: " << time_diff << endl;


        // TRACKING
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
            trackingWindow->setPixmap(QPixmap::fromImage(imdisplay3));
        }
    }
}

void Camera::RecordVideo() {
    int ReturnVal = 0;
    while (true) {
        // TO DO: Set this up so it will only log for debug mode
       // timeval startTime, endTime;
      //  gettimeofday(&startTime, NULL);

        // this part exits loop if bool is set to true
        //QMutexLocker locker(mutex);
        if (loop_stopped) {
            break;
        }
        ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
        if (ReturnVal) {
            printf("EVT_CameraQueueFrame Error!\n");
            return;
        }


        cv::Mat frame;
        if (format=="RGB8") {
            frame = cv::Mat(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC3, evtFrameRecv.imagePtr);
        } else if (format == "BayerRGB") {
            cv::Mat frameConvert(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC1, evtFrameRecv.imagePtr);
            cv::cvtColor(frameConvert,frame,COLOR_BayerRG2RGB );
        } else {
            cv::Mat frameConvert(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC2, evtFrameRecv.imagePtr);
            cv::cvtColor(frameConvert,frame,COLOR_YUV2BGR_Y422 );
        }
        writer.write(frame);


      //  gettimeofday(&endTime, NULL);
     //   float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
     //   cout << "Time of Conversion: " << time_diff << endl;
        ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
    }
}

void Camera::setTrackingWindow(QLabel *value)
{
    trackingWindow = value;
}

void Camera::setBackgroundWindow(QLabel *value)
{
    backgroundWindow = value;
}

QString Camera::getOutputLocation() const
{
    return outputLocation;
}

void Camera::setOutputLocation(const QString &value)
{
    outputLocation = value;
}

QString Camera::getName() const
{
    return name;
}

void Camera::setPreviewFrame(QLabel *value)
{
    previewFrame = value;
}

void Camera::GrabBackgroundFrame() {
    frameGrabFrame = currFrame.clone();
    cv::Mat temp;
    cv::resize(frameGrabFrame, temp, Size(261, 161), 0, 0, INTER_LINEAR);

    QImage imdisplay2((uchar*)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    backgroundWindow->setPixmap(QPixmap::fromImage(imdisplay2));
}


void Camera::StopCamera() {
    this->moveToThread(QApplication::instance()->thread());
    //QMutexLocker locker(mutex);
    loop_stopped = true;

    EVT_CameraExecuteCommand(&camera, "AcquisitionStop");
}


