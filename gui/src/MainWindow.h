#pragma once

#include <QMainWindow>

class QTabWidget;
class ConfigTab;
class ExperimentTab;

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  QTabWidget *tabs_ = nullptr;
  ConfigTab *initTab_ = nullptr;
  ExperimentTab *experimentTab_ = nullptr;
};
