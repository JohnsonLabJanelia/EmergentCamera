#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <opencv2/opencv.hpp>
#include <EmergentCamera.h>
#include <emergentframe.h>
#include <camerasettings.h>
#include <gigevisiondeviceinfo.h>


using namespace Emergent;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public slots:
    void DisplayPreview();    
    void PipeCameraFrame();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void ParseOptionsFile();
    bool CheckEmergentCamera(GigEVisionDeviceInfo* deviceInfo);
    void ConfigureEmergentCameraDefaults(Emergent::CEmergentCamera *camera);
    QString GenerateGStreamerPipeline();
    QImage imdisplay;

    void SetupPreview();
private slots:
    void on_loadOptions_clicked();

    void on_saveLocation_clicked();
    
    void on_saveOptions_clicked();

    void on_recordButton_clicked();

    void on_previewButton_clicked();

private:
    Ui::MainWindow *ui;
    cv::VideoCapture cap;
    cv::VideoWriter writer;
    QTimer* timer;
    CameraSettings* settings;
    CEmergentCamera camera;
    CEmergentFrame evtFrame[30], evtFrameRecv;
    bool recording = false;
    bool preview = false;
};

#endif // MAINWINDOW_H
