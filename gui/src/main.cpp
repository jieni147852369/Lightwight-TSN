#include <QApplication>

#include "MainWindow.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("TSN_SoftWare");
  QCoreApplication::setApplicationVersion("0.1");

  MainWindow window;
  window.resize(980, 720);
  window.show();
  return app.exec();
}
