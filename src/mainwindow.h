#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QString>
#include <QCheckBox>
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
#include <camera.h>
#include <opencv2/core/cvstd_wrapper.hpp>

using namespace cv;
using namespace Emergent;
#define MAX_WORKERS 8
#define MAX_CAMERAS 6
#define MAX_FRAMES 1000
#define FRAMES_BUFFERS 30
#define AVI_FRAME_COUNT 200 //Number of frames to save to AVI.


struct worker {
    pthread_t  thread_id;
    int    worker_id;
};
typedef struct worker worker_t;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void ParseOptionsFile();
    QString GenerateGStreamerPipeline();
private slots:
    void on_loadOptions_clicked();

    void on_saveLocation_clicked();
    
    void on_saveOptions_clicked();

    void on_recordButton_clicked();

    void on_previewButton_clicked();

    void on_frameGrabButton_clicked();

    void on_frameRateLineEdit_editingFinished();

    void on_SetCameraButton_clicked();

private:
    Ui::MainWindow *ui;
    cv::VideoCapture cap;

    bool CheckEmergentCamera(GigEVisionDeviceInfo* deviceInfo);
    FILE *pipeout;
    unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate;
    unsigned int height_max, width_max;
    QTimer* timer;
    CameraSettings* settings;
    Ptr<BackgroundSubtractor> backSub;
    Camera camera[MAX_CAMERAS];
    QCheckBox *cameraSelector[MAX_CAMERAS];
    unsigned int cameras_found;
    CEmergentFrame evtFrameRecv, evtFrameConvert;
    QString format;
    bool recording = false;
    bool preview = false;
    QString baseDirectory;
    QLabel* previewWindow[4];
    QLabel* backgroundWindow[4];
    QLabel* trackingWindow[4];
    int previewCounter = 0;
};

#endif // MAINWINDOW_H
