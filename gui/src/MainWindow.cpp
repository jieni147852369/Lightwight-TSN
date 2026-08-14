#include "MainWindow.h"

#include <QTabWidget>
#include <QWidget>

#include "ConfigTab.h"
#include "ExperimentTab.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("轻量时间敏感网络软件");

  tabs_ = new QTabWidget(this);
  setCentralWidget(tabs_);

  initTab_ = new ConfigTab(tabs_);
  tabs_->addTab(initTab_, "配置");

  experimentTab_ = new ExperimentTab(tabs_);
  tabs_->addTab(experimentTab_, "实验");
}
