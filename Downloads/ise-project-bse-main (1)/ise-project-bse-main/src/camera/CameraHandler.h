#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QString>

class QCamera;
class QMediaCaptureSession;
class QVideoFrame;
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

    // Most recent frame grabbed from the preview, or a null image if the
    // camera has not delivered a frame yet.
    QImage lastFrameImage() const;
    void clearLastFrame();

signals:
    void errorOccurred(const QString &message);

private:
    void handleFrame(const QVideoFrame &frame);

    QCamera *camera;
    QMediaCaptureSession *captureSession;
    QVideoWidget *viewfinder;
    QImage lastFrame;
    QElapsedTimer frameTimer;
};

#endif
