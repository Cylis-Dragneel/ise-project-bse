#include "CameraHandler.h"
#include <QMediaDevices>
#include <QDebug>

CameraHandler::CameraHandler(QVideoWidget *viewfinderWidget, QObject *parent)
: QObject(parent), camera(nullptr), captureSession(nullptr), viewfinder(viewfinderWidget)
{}

CameraHandler::~CameraHandler()
{
    stopCamera();
}

void CameraHandler::startCamera()
{
    if (!camera)
    {
        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty())
        {
            qWarning() << "No video input devices found.";
            return;
        }
        
        //Initialize default video input device
        camera = new QCamera(cameras.first(), this);
        captureSession = new QMediaCaptureSession(this);
        
        captureSession->setCamera(camera);
        if (viewfinder)
        {
            captureSession->setVideoOutput(viewfinder);
        }
    }

    if (camera && !camera->isActive())
    {
        camera->start();
    }
}

void CameraHandler::stopCamera()
{
    if (camera && camera->isActive())
    {
        camera->stop();
    }
}

bool CameraHandler::isRunning() const
{
    return camera && camera->isActive();
}
