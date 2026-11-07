#include <QApplication>
#include <QLabel>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QLabel w("QR App skeleton");
  w.resize(400, 200);
  w.show();
  return app.exec();
}
