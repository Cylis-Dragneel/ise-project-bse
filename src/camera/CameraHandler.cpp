#include "CameraHandler.h"

#include <QCamera>
#include <QDebug>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QVideoWidget>

CameraHandler::CameraHandler(QVideoWidget *viewfinderWidget, QObject *parent)
    : QObject(parent), camera(nullptr), captureSession(nullptr),
      viewfinder(viewfinderWidget)
{
}

CameraHandler::~CameraHandler()
{
    stopCamera();
}

bool CameraHandler::startCamera()
{
    if (!camera) {
        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            qWarning() << "No video input devices found.";
            emit errorOccurred(
                "No camera was found. Connect a webcam and try again.");
            return false;
        }

        // Initialize the default video input device.
        camera = new QCamera(cameras.first(), this);

        connect(camera, &QCamera::errorOccurred, this,
                [this](QCamera::Error error, const QString &errorString) {
                    if (error == QCamera::NoError) {
                        return;
                    }

                    emit errorOccurred(
                        errorString.isEmpty()
                            ? QString("The camera reported an error.")
                            : errorString);
                });

        captureSession = new QMediaCaptureSession(this);
        captureSession->setCamera(camera);

        if (viewfinder) {
            captureSession->setVideoOutput(viewfinder);
        }
    }

    if (!camera->isActive()) {
        camera->start();
    }

    return true;
}

void CameraHandler::stopCamera()
{
    if (camera && camera->isActive()) {
        camera->stop();
    }
}

bool CameraHandler::isRunning() const
{
    return camera && camera->isActive();
}
