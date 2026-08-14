#pragma once

#include <QWidget>

class QTabWidget;
class SenderWidget;
class ReceiverWidget;

class ExperimentTab final : public QWidget {
  Q_OBJECT

 public:
  explicit ExperimentTab(QWidget *parent = nullptr);

 private:
  QTabWidget *tabs_ = nullptr;
  SenderWidget *sender_ = nullptr;
  ReceiverWidget *receiver_ = nullptr;
};
