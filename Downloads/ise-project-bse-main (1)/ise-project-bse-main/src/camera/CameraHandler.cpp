#include "CameraHandler.h"

#include <QCamera>
#include <QDebug>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVideoWidget>

namespace {
// Converting every camera frame to an image is expensive and unnecessary:
// only the most recent snapshot is needed when the user stops the camera.
constexpr qint64 kFrameIntervalMs = 200;
}

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

            // The widget owns the sink that renders the preview; tapping it
            // gives us frames without a second video output.
            if (QVideoSink *sink = viewfinder->videoSink()) {
                connect(sink, &QVideoSink::videoFrameChanged,
                        this, &CameraHandler::handleFrame);
            }
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

QImage CameraHandler::lastFrameImage() const
{
    return lastFrame;
}

void CameraHandler::clearLastFrame()
{
    lastFrame = QImage();
    frameTimer.invalidate();
}

void CameraHandler::handleFrame(const QVideoFrame &frame)
{
    // Throttle the conversion; the preview itself still runs at full rate.
    if (frameTimer.isValid() && frameTimer.elapsed() < kFrameIntervalMs) {
        return;
    }
    frameTimer.start();

    QVideoFrame readable(frame);
    if (!readable.map(QVideoFrame::ReadOnly)) {
        return;
    }

    const QImage image = readable.toImage();
    readable.unmap();

    if (!image.isNull()) {
        lastFrame = image;
    }
}
