#include "ExperimentTab.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "ReceiverWidget.h"
#include "SenderWidget.h"

ExperimentTab::ExperimentTab(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  tabs_ = new QTabWidget(this);
  layout->addWidget(tabs_);

  sender_ = new SenderWidget(tabs_);
  receiver_ = new ReceiverWidget(tabs_);
  tabs_->addTab(sender_, "发帧");
  tabs_->addTab(receiver_, "接收");
}
