#ifndef CAMERA_H
#define CAMERA_H

#include <QMainWindow>
#include <QString>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/bgsegm.hpp>
#include <EmergentCamera.h>
#include <emergentframe.h>
#include <EmergentAVIFile.h>
#include <camerasettings.h>
#include <gigevisiondeviceinfo.h>
#include <unistd.h>
#include <sys/time.h>
#include <QLabel>
#include <QTimer>
#include <QObject>
#include <opencv2/core/cvstd_wrapper.hpp>

using namespace cv;
using namespace Emergent;
// Contains all the information associated with an emergent camera, including deviceInfo, settings, temporary frame data, and preview OpenCV image data
class Camera : public QObject
{
public slots:
    void DisplayPreview();
    void RecordVideo();
    void StopCamera();
private:
    Q_OBJECT
    QTimer *timer;
    unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate;
    unsigned int height_max, width_max;
    int deviceInfoIndex = 0;
    CEmergentCamera camera;
    CEmergentFrame evtFrame[30];
    cv::VideoWriter writer;
    CEmergentFrame evtFrameRecv, evtFrameConvert;
    cv::Mat currFrame, frameGrabFrame;
    QString format;
    QString name;
    QString outputLocation;
    int MAX_CAMERAS=6;
    int numCameras = 0;
    QLabel *previewFrame;
    QLabel *backgroundWindow;
    QLabel *trackingWindow;
    QMutex *mutex;
    bool loop_stopped;
public:
    explicit Camera(QObject * parent = nullptr);
    void SetupCamera(GigEVisionDeviceInfo* deviceInfo);
    void InitRecord(QString recordOptions, int fps);
    void ReleaseWriter();
    void cleanUpCamera();
    int GetDeviceInfoIndex();
    void SetDeviceInfoIndex(int index);
    QString getFormat() const;
    QString getCameraName() const;
    void setFormat(const QString &value);
    void setFrameRate(int frameRate);
    int getFrameRate();
    void ConfigureEmergentCameraDefaults(CEmergentCamera *camera);
    CEmergentCamera* getEmergentCameraPtr();
    void ReleaseFrame(int frameIndex);
    void setPreviewFrame(QLabel *value);

    void StartRecord();
    QString getName() const;
    QString getOutputLocation() const;
    void setOutputLocation(const QString &value);
    void setBackgroundWindow(QLabel *value);
    void setTrackingWindow(QLabel *value);
    void GrabBackgroundFrame();
};

#endif // CAMERA_H
