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
#include <emergentcameradef.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentCamera.h>
#include <unistd.h>

using namespace cv;
using namespace Emergent;


struct G {
    pthread_mutex_t wrk_mtx;
    worker_t worker[MAX_WORKERS];
    bool done;
    unsigned int frame_count;
    unsigned int appended_frame_count;
    unsigned int frame_to_recv;
    unsigned int frame_id_prev;
    CEmergentCamera *p_camera;
    unsigned short id_prev;
    unsigned int dropped_frames;
    CEmergentAVIFile aviFile;
    unsigned int width;
    unsigned int height;
    PIXEL_FORMAT pixel_type;
    bool avi_open;
} G;

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

bool MainWindow::Process_Frame(CEmergentFrame* frame, CEmergentFrame* evtFrameConvert) {
    bool converted = true;

    switch(frame->pixel_type)
    {
    case GVSP_PIX_MONO8:
    {
        //N/A: no convert required.
        converted = false;
        break;
    }

    case GVSP_PIX_BAYGB8:
    {
        //EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //Bayer interpolate
        //Will do this raw for speed
        //If converting then need to set G.aviFile.isColor = true; above.
        converted = false;
        break;
    }

    case GVSP_PIX_BAYGR8:
    {
        //EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //Bayer interpolate
        //Will do this raw for speed
        //If converting then need to set G.aviFile.isColor = true; above.
        converted = false;
        break;
    }

    case GVSP_PIX_BAYRG8:
    {
        //EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //Bayer interpolate
        //Will do this raw for speed
        //If converting then need to set G.aviFile.isColor = true; above.
        converted = false;
        break;
    }

    case GVSP_PIX_BAYBG8:
    {
        //EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //Bayer interpolate
        //Will do this raw for speed
        //If converting then need to set G.aviFile.isColor = true; above.
        converted = false;
        break;
    }

    case GVSP_PIX_BAYGB10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYGR10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYRG10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYBG10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYGB12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYGR12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYRG12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_BAYBG12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NEARESTNEIGHBOR_BGR); //8 bit for AVI, Bayer interpolate.
        break;
    }

    case GVSP_PIX_RGB8:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR);  //BGR for AVI
        break;
    }

    case GVSP_PIX_RGB10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_TO_BGR);  //8 bit for AVI, BGR for AVI
        break;
    }

    case GVSP_PIX_RGB12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_TO_BGR);  //8 bit for AVI, BGR for AVI
        break;
    }

    case GVSP_PIX_BGR8:
    {
        //N/A: no convert required.
        converted = false;
        break;
    }

    case GVSP_PIX_BGR10:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE);   //8 bit for AVI.
        break;
    }

    case GVSP_PIX_BGR12:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE);   //8 bit for AVI.
        break;
    }

    case GVSP_PIX_YUV411_PACKED:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
        break;
    }

    case GVSP_PIX_YUV422_PACKED:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
        break;
    }

    case GVSP_PIX_YUV444_PACKED:
    {
        EVT_FrameConvert(frame, evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
        break;
    }

    default:
        break;
    }

    return converted;
}

void MainWindow::AVIWorkThread(void *work_struct) {

    worker_t *wrk;
    wrk = (worker_t *)work_struct;

    int ReturnVal = 0;
    int SUCCESS = 0;
    CEmergentFrame evtFrameRecv, evtFrameConvert, *evtFrameForAVI = NULL;
    bool buffer_used = false, buffer_recd = false, converted;
    unsigned int frame_count_this = 0;

    printf("Worker Thread %d Started...\n", wrk->worker_id);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(wrk->worker_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    //Our single convert buffer needs to be allocated.
    evtFrameConvert.size_x = G.width;
    evtFrameConvert.size_y = G.height;
    evtFrameConvert.pixel_type = G.pixel_type;
    evtFrameConvert.convertColor = EVT_COLOR_CONVERT_BILINEAR;
    evtFrameConvert.convertBitDepth = EVT_CONVERT_8BIT;
    EVT_AllocateFrameBuffer(G.p_camera, &evtFrameConvert, EVT_FRAME_BUFFER_DEFAULT);

    while (!G.done) {
        //Lock the frame resource.
        pthread_mutex_lock(&G.wrk_mtx);

        if (G.done) {
            pthread_mutex_unlock(&G.wrk_mtx);
            break;
        }

        //Re-queue frame if we have received one which is done being processed.
        if (buffer_used) {
            //Re-queue. Still need to re-queue if dropped frame due to frame_id check.
            ReturnVal = EVT_CameraQueueFrame(G.p_camera, &evtFrameRecv);
            if(ReturnVal) printf("EVT_CameraQueueFrame Error!\n");
            buffer_used = false;
        }

        //Get the next buffer.
        ReturnVal = EVT_CameraGetFrame(G.p_camera, &evtFrameRecv, EVT_INFINITE);
        if (!ReturnVal) {
            //Counting dropped frames through frame_id as redundant check.
            //Ignore very first frame as id is unknown.
            if (((evtFrameRecv.frame_id) != G.id_prev+1) && (G.frame_count != 0)) {
                G.dropped_frames++;
                buffer_recd = false;
                //printf("\ndfe!\n");
                //printf("%d %d %d\n", evtFrameRecv.frame_id, G.id_prev, G.frame_count);
            } else {
                G.frame_count++;
                buffer_recd = true;
            }
        } else {
            G.dropped_frames++;
            buffer_recd = false;
        }

        //In GVSP there is no id 0 so when 16 bit id counter in camera is max then the next id is 1 so set prev id to 0 for math above.
        if(evtFrameRecv.frame_id == 65535)
            G.id_prev = 0;
        else
            G.id_prev = evtFrameRecv.frame_id;

        //Indicate buffer has been used so at top of while we can re-queue.
        buffer_used = true;

        //Check if we have received ALL frames amongst worker threads.
        if (G.frame_count >= G.frame_to_recv)
            G.done = true;

        //Others will change this when outside mtx so save it for AVI sake.
        frame_count_this = G.frame_count;

        //Unlock the frame resource so other workers can access.
        pthread_mutex_unlock(&G.wrk_mtx);

        //Don't process further if frame dropped.
        if(!buffer_recd) continue;

        //Process the block here.
        converted = Process_Frame(&evtFrameRecv, &evtFrameConvert);

        //Determine frame to use for AVI save. Process_Frame converts or not depending on pixel format.
        if (converted)
            evtFrameForAVI = &evtFrameConvert;
        else
            evtFrameForAVI = &evtFrameRecv;

        //Wait for threads turn to append. Helps keep frames in order for append.
        while(frame_count_this != G.appended_frame_count + 1);

        //Every AVI_FRAME_COUNT frames open AVI file again.
        if (((frame_count_this-1)%AVI_FRAME_COUNT) == 0) {
            if (!G.avi_open) {
                EVT_AVIOpen(&G.aviFile);
                EVT_AVIAppend(&G.aviFile, evtFrameForAVI);
                printf("o.");
                G.avi_open = true;
            }
        } else if(((frame_count_this-1)%AVI_FRAME_COUNT) == AVI_FRAME_COUNT-1) {
            //Every AVI_FRAME_COUNT frames close AVI file again.
            if(G.avi_open) {
                EVT_AVIAppend(&G.aviFile, evtFrameForAVI);
                EVT_AVIClose(&G.aviFile);
                printf("c.");
                G.avi_open = false;
            }
        } else {
            if (G.avi_open) {
                EVT_AVIAppend(&G.aviFile, evtFrameForAVI);
            }
        }

        //Incrementing this counter will allow next thread to do its append.
        G.appended_frame_count++;
    }

    printf("\nWorker Thread %d Ended...", wrk->worker_id);
}

bool MainWindow::SetupAviWorkers() {
   int fps = QJsonValue::fromVariant(ui->fps_options->itemData(ui->fps_options->currentIndex())).toInt();
   G.aviFile.fps       = fps;
   G.aviFile.codec     = EVT_CODEC_NONE;

   int width = 3208;
   int height = 2200;
   unsigned int numworkers;

   // hardcode for now to get basic threading example working
   G.aviFile.width     = 3208;
   G.aviFile.height    = 2200;

   if (pthread_mutex_init(&G.wrk_mtx, NULL) != 0) {
       printf("\n mutex init failed\n");
       return false;
   }

   int dwNumberOfProcessors = sysconf( _SC_NPROCESSORS_ONLN );

   //Not done before starting threads.
   G.done = false;
   G.frame_count = 0;
   G.appended_frame_count = 0;
   // need to figure out how to interrupt worker threads from Qt main GUI
   G.frame_to_recv = MAX_FRAMES;
   G.id_prev = 0;
   G.dropped_frames = 0;
   G.width = width;
   G.height = height;
   G.pixel_type = evtFrame[0].pixel_type;
   G.avi_open = false;

   //Start as many worker threads as we have processors.
   if(dwNumberOfProcessors < MAX_WORKERS)
       numworkers = dwNumberOfProcessors;
   else
       numworkers = MAX_WORKERS;

   for(unsigned int i=0; i < numworkers; i++) {
       G.worker[i].worker_id = i;
       G.p_camera = &camera;
       pthread_create(&G.worker[i].thread_id, NULL, (void* (*)(void*))&AVIWorkThread, (void*)(&G.worker[i]));
   }
   return true;

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

void MainWindow::SetupCamera() {
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
        EVT_CameraSetEnumParam(&camera,      "PixelFormat", "Mono8");
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
            evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;
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
   // EVT_FrameConvert(&evtFrameRecv, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE);
    //EVT_FrameSave(&evtFrameRecv, "pre_opencv_esdk.tif", EVT_FILETYPE_TIF, EVT_ALIGN_NONE);

     cv::Mat frame(evtFrameRecv.size_y, evtFrameRecv.size_x, CV_8UC1);
     cv::resize(frame, frame, Size(561, 316), 0, 0, INTER_LINEAR);

     QImage imdisplay((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Indexed8);
     ui->VideoPreview->setPixmap(QPixmap::fromImage(imdisplay));

     ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.
}

void MainWindow::PipeCameraFrame() {
    int ReturnVal = 0;
    ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);
    if (ReturnVal) {
        printf("EVT_CameraQueueFrame Error!\n");
        return;
    }
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
            ui->h264_checkbox->setChecked(true);
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
    if (ui->h264_checkbox->isChecked()) {
        compression = "h264";
    }
    cameraOptions["compression"] = compression;
    QJsonDocument optionsDocument(cameraOptions);
    file.write(optionsDocument.toJson());
}

void MainWindow::on_recordButton_clicked()
{
    if (!recording) {
        recording = true;

        // switch icon for button to stop
        ui->recordButton->setIcon(QIcon::fromTheme("media-playback-stop"));

        timer = new QTimer (this);
        connect(timer, SIGNAL(timeout()), this, SLOT(PipeCameraFrame()));
        timer->start((int)1000.0/settings->getFPS().toDouble());
    } else {
        recording = false;

        // switch icon for button to record
        ui->recordButton->setIcon(QIcon::fromTheme("media-record"));

        timer->stop();
        EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

        for(int frame_count=0;frame_count<30;frame_count++) {
                EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
        }

        EVT_CameraCloseStream(&camera);
        EVT_CameraClose(&camera);
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
        timer->start(30);
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
