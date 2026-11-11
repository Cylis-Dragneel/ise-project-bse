#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <QObject>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>

class CameraHandler : public QObject {
    Q_OBJECT
public:
    explicit CameraHandler(QVideoWidget *viewfinderWidget, QObject *parent = nullptr);
    ~CameraHandler();

    void startCamera();
    void stopCamera();
    bool isRunning() const;

private:
    QCamera *camera;
    QMediaCaptureSession *captureSession;
    QVideoWidget *viewfinder;
};

#endif
