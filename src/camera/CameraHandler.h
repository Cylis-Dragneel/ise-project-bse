#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <QObject>
#include <QString>

class QCamera;
class QMediaCaptureSession;
class QVideoWidget;

class CameraHandler : public QObject {
    Q_OBJECT
public:
    explicit CameraHandler(QVideoWidget *viewfinderWidget, QObject *parent = nullptr);
    ~CameraHandler() override;

    // Returns false when no camera device is available.
    bool startCamera();
    void stopCamera();
    bool isRunning() const;

signals:
    void errorOccurred(const QString &message);

private:
    QCamera *camera;
    QMediaCaptureSession *captureSession;
    QVideoWidget *viewfinder;
};

#endif
