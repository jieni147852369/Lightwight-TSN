#include "ReceiverWidget.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

#include <unistd.h>

static QString ifaceLabel(const QNetworkInterface &iface) {
  const bool up = iface.flags().testFlag(QNetworkInterface::IsUp);
  return QString("%1 (%2)").arg(iface.name(), up ? "up" : "down");
}

ReceiverWidget::ReceiverWidget(QWidget *parent) : QWidget(parent) {
  auto *root = new QVBoxLayout(this);

  auto *fileBox = new QGroupBox("配置文件", this);
  auto *fileLayout = new QHBoxLayout(fileBox);
  configPath_ = new QLineEdit(resolveDefaultConfigPath(), fileBox);
  btnBrowse_ = new QPushButton("选择...", fileBox);
  btnOpen_ = new QPushButton("打开", fileBox);
  btnSave_ = new QPushButton("保存", fileBox);
  btnSaveAs_ = new QPushButton("另存为", fileBox);
  fileLayout->addWidget(new QLabel("路径：", fileBox));
  fileLayout->addWidget(configPath_, 1);
  fileLayout->addWidget(btnBrowse_);
  fileLayout->addWidget(btnOpen_);
  fileLayout->addWidget(btnSave_);
  fileLayout->addWidget(btnSaveAs_);
  root->addWidget(fileBox);

  auto *cfgBox = new QGroupBox("接收配置", this);
  auto *form = new QFormLayout(cfgBox);
  iface_ = new QComboBox(cfgBox);
  iface_->setEditable(true);
  btnReloadIfaces_ = new QPushButton("刷新接口", cfgBox);
  auto *ifaceRow = new QWidget(cfgBox);
  auto *ifaceRowLayout = new QHBoxLayout(ifaceRow);
  ifaceRowLayout->setContentsMargins(0, 0, 0, 0);
  ifaceRowLayout->addWidget(iface_, 1);
  ifaceRowLayout->addWidget(btnReloadIfaces_);
  form->addRow("interface", ifaceRow);

  port_ = new QSpinBox(cfgBox);
  port_->setRange(1, 65535);
  form->addRow("port", port_);
  root->addWidget(cfgBox);

  auto *ctrl = new QHBoxLayout();
  btnStart_ = new QPushButton("启动", this);
  btnStop_ = new QPushButton("停止", this);
  btnClear_ = new QPushButton("清空帧信息", this);
  btnExport_ = new QPushButton("导出TXT", this);
  btnStop_->setEnabled(false);
  ctrl->addWidget(btnStart_);
  ctrl->addWidget(btnStop_);
  ctrl->addStretch(1);
  ctrl->addWidget(btnClear_);
  ctrl->addWidget(btnExport_);
  root->addLayout(ctrl);

  auto *statsRow = new QWidget(this);
  auto *statsLayout = new QHBoxLayout(statsRow);
  statsLayout->setContentsMargins(0, 0, 0, 0);
  stats_ = new QLabel(statsRow);
  stats_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  logPath_ = new QLabel(statsRow);
  logPath_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  statsLayout->addWidget(stats_, 0);
  statsLayout->addSpacing(12);
  statsLayout->addWidget(logPath_, 1);
  root->addWidget(statsRow);

  output_ = new QTableWidget(this);
  output_->setColumnCount(10);
  output_->setHorizontalHeaderLabels(QStringList() << "time"
                                                   << "seq"
                                                   << "len"
                                                   << "prio"
                                                   << "src_ip"
                                                   << "src_port"
                                                   << "expected_send_ns"
                                                   << "rx_ts_ns"
                                                   << "rx_ts_src"
                                                   << "latency");
  output_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  output_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  output_->setSelectionBehavior(QAbstractItemView::SelectRows);
  root->addWidget(output_, 3);

  connect(btnBrowse_, &QPushButton::clicked, this, &ReceiverWidget::browseConfig);
  connect(btnOpen_, &QPushButton::clicked, this, &ReceiverWidget::loadConfig);
  connect(btnSave_, &QPushButton::clicked, this, &ReceiverWidget::saveConfig);
  connect(btnSaveAs_, &QPushButton::clicked, this, &ReceiverWidget::saveConfigAs);
  connect(btnReloadIfaces_, &QPushButton::clicked, this, &ReceiverWidget::reloadInterfaces);
  connect(btnStart_, &QPushButton::clicked, this, &ReceiverWidget::startReceiver);
  connect(btnStop_, &QPushButton::clicked, this, &ReceiverWidget::stopReceiver);
  connect(btnClear_, &QPushButton::clicked, this, &ReceiverWidget::clearOutput);
  connect(btnExport_, &QPushButton::clicked, this, &ReceiverWidget::exportOutputTxt);

  flushTimer_ = new QTimer(this);
  flushTimer_->setInterval(30);
  connect(flushTimer_, &QTimer::timeout, this, &ReceiverWidget::flushPendingRows);
  flushTimer_->start();

  reloadInterfaces();
  loadConfig();
  updateStatsUi();
}

QString ReceiverWidget::resolveDefaultConfigPath() const {
  const QStringList candidates = {
      QDir(QCoreApplication::applicationDirPath()).filePath("../configs/tsn_multi_receiver.conf"),
      QDir::current().filePath("configs/tsn_multi_receiver.conf"),
      QDir::current().filePath("../configs/tsn_multi_receiver.conf"),
  };
  for (const auto &p : candidates) {
    if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
  }
  return QDir::current().filePath("tsn_multi_receiver.conf");
}

QString ReceiverWidget::resolveReceiverBinary() const {
  const QStringList candidates = {
      QDir(QCoreApplication::applicationDirPath()).filePath("tsn_multi_receiver"),
      QDir::current().filePath("bin/tsn_multi_receiver"),
      QDir::current().filePath("../bin/tsn_multi_receiver"),
  };
  for (const auto &p : candidates) {
    if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
  }
  return QString();
}

void ReceiverWidget::reloadInterfaces() {
  const QString prev = iface_->currentData().toString();
  iface_->clear();
  for (const auto &iface : QNetworkInterface::allInterfaces()) {
    if (iface.name() == "lo") continue;
    iface_->addItem(ifaceLabel(iface), iface.name());
  }
  int idx = prev.isEmpty() ? 0 : iface_->findData(prev);
  if (idx < 0) idx = 0;
  iface_->setCurrentIndex(idx);
}

void ReceiverWidget::browseConfig() {
  const QString file = QFileDialog::getOpenFileName(
      this, "选择接收配置文件", QFileInfo(configPath_->text()).absolutePath(),
      "Config (*.conf *.txt);;All Files (*)");
  if (file.isEmpty()) return;
  configPath_->setText(file);
  loadConfig();
}

bool ReceiverWidget::fillUiFromConfig(const ReceiverConfig &cfg) {
  const int idx = iface_->findData(cfg.interface);
  if (idx >= 0) iface_->setCurrentIndex(idx);
  port_->setValue(cfg.port);
  return true;
}

void ReceiverWidget::loadConfig() {
  QString error;
  ReceiverConfig cfg;
  if (!ReceiverConfigIo::loadFromFile(configPath_->text().trimmed(), &cfg, &error)) {
    QMessageBox::warning(this, "打开失败", error);
    return;
  }
  fillUiFromConfig(cfg);
}

ReceiverConfig ReceiverWidget::configFromUi(QString *error) const {
  ReceiverConfig cfg;
  cfg.interface = iface_->currentData().toString().trimmed();
  if (cfg.interface.isEmpty()) cfg.interface = iface_->currentText().trimmed();
  cfg.port = port_->value();
  if (cfg.interface.isEmpty()) {
    if (error) *error = "interface 不能为空";
    return {};
  }
  if (cfg.port <= 0 || cfg.port > 65535) {
    if (error) *error = "port 范围应为 1..65535";
    return {};
  }
  return cfg;
}

void ReceiverWidget::saveConfig() {
  QString error;
  const auto cfg = configFromUi(&error);
  if (!error.isEmpty()) {
    QMessageBox::warning(this, "保存失败", error);
    return;
  }
  if (!ReceiverConfigIo::saveToFile(configPath_->text().trimmed(), cfg, &error)) {
    QMessageBox::warning(this, "保存失败", error);
    return;
  }
}

void ReceiverWidget::saveConfigAs() {
  const QString file = QFileDialog::getSaveFileName(
      this, "另存为接收配置", QFileInfo(configPath_->text()).absolutePath(),
      "Config (*.conf *.txt);;All Files (*)");
  if (file.isEmpty()) return;
  configPath_->setText(file);
  saveConfig();
}

void ReceiverWidget::startReceiver() {
  if (proc_) {
    QMessageBox::information(this, "提示", "接收程序已在运行。");
    return;
  }
  if (geteuid() != 0) {
    QMessageBox::warning(this, "权限不足", "建议用 sudo 启动 TSN_SoftWare 后再启动接收。");
    return;
  }

  QString error;
  const auto cfg = configFromUi(&error);
  if (!error.isEmpty()) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }
  if (!ReceiverConfigIo::saveToFile(configPath_->text().trimmed(), cfg, &error)) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }

  const QString bin = resolveReceiverBinary();
  if (bin.isEmpty()) {
    QMessageBox::warning(this, "启动失败",
                         "找不到接收程序：bin/tsn_multi_receiver（请先在 TSN_SoftWare 目录 qmake+make 生成）");
    return;
  }

  clearOutput();
  pendingRows_.clear();
  totalMatched_ = 0;
  droppedUi_ = 0;
  lastSeq_.clear();
  statsDirty_ = true;
  logWriteBuffer_.clear();
  if (logFile_) {
    logFile_->close();
    logFile_->deleteLater();
    logFile_ = nullptr;
  }

  {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    const QString fn = QString("tsn_multi_receiver_%1.tsv")
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmsszzz"));
      logFilePath_ = QDir(dir).filePath(fn);
      logFile_ = new QFile(logFilePath_, this);
      if (!logFile_->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        logFile_->deleteLater();
        logFile_ = nullptr;
      logFilePath_.clear();
    } else {
      const QStringList headers = QStringList() << "time"
                                                << "seq"
                                                << "len"
                                                << "prio"
                                                << "src_ip"
                                                << "src_port"
                                                << "expected_send_ns"
                                                << "rx_ts_ns"
                                                << "rx_ts_src"
                                                << "latency";
      logFile_->write(headers.join('\t').toUtf8());
      logFile_->write("\n");
    }
  }

  proc_ = new QProcess(this);
  QString program = bin;
  QStringList args = QStringList() << configPath_->text().trimmed();
  const QString stdbuf = QStandardPaths::findExecutable("stdbuf");
  if (!stdbuf.isEmpty()) {
    program = stdbuf;
    args = QStringList() << "-oL"
                         << "-eL" << bin << configPath_->text().trimmed();
  }
  proc_->setProgram(program);
  proc_->setArguments(args);
  proc_->setProcessChannelMode(QProcess::SeparateChannels);
  connect(proc_, &QProcess::readyReadStandardOutput, this, &ReceiverWidget::onStdout);
  connect(proc_, &QProcess::readyReadStandardError, this, &ReceiverWidget::onStderr);
  connect(proc_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ReceiverWidget::onFinished);
  proc_->start();
  if (!proc_->waitForStarted(3000)) {
    QMessageBox::warning(this, "启动失败", "无法启动接收进程。");
    proc_->deleteLater();
    proc_ = nullptr;
    return;
  }

  btnStart_->setEnabled(false);
  btnStop_->setEnabled(true);
  updateStatsUi();
}

void ReceiverWidget::stopReceiver() {
  if (!proc_) return;
  proc_->terminate();
  if (!proc_->waitForFinished(1500)) proc_->kill();
}

void ReceiverWidget::clearOutput() {
  output_->setRowCount(0);
  statsDirty_ = true;
  updateStatsUi();
}

void ReceiverWidget::exportOutputTxt() {
  const QString file = QFileDialog::getSaveFileName(
      this, "导出接收信息 TXT", QDir::currentPath(), "Text (*.txt);;All Files (*)");
  if (file.isEmpty()) return;

  if (logFile_) {
    flushLogBuffer();
    logFile_->flush();
  }

  if (!logFilePath_.isEmpty() && QFileInfo::exists(logFilePath_)) {
    QFile src(logFilePath_);
    if (!src.open(QIODevice::ReadOnly)) {
      QMessageBox::warning(this, "导出失败", "无法读取日志文件。");
      return;
    }
    QFile dst(file);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QMessageBox::warning(this, "导出失败", "无法写入文件。");
      return;
    }
    while (!src.atEnd()) {
      const QByteArray chunk = src.read(1024 * 1024);
      if (chunk.isEmpty()) break;
      if (dst.write(chunk) != chunk.size()) {
        QMessageBox::warning(this, "导出失败", "写入中断。");
        return;
      }
    }
    return;
  }

  QFile f(file);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QMessageBox::warning(this, "导出失败", "无法写入文件。");
    return;
  }
  QTextStream out(&f);
  QStringList headers;
  for (int c = 0; c < output_->columnCount(); ++c) headers << output_->horizontalHeaderItem(c)->text();
  out << headers.join('\t') << "\n";
  for (int r = 0; r < output_->rowCount(); ++r) {
    QStringList cols;
    for (int c = 0; c < output_->columnCount(); ++c) {
      auto *it = output_->item(r, c);
      cols << (it ? it->text() : "");
    }
    out << cols.join('\t') << "\n";
  }
}

void ReceiverWidget::onStdout() {
  if (!proc_) return;
  const auto lines = stdoutBuf_.append(proc_->readAllStandardOutput());
  for (const auto &l : lines) appendReceiverLine(l);
}

void ReceiverWidget::onStderr() {
  if (!proc_) return;
  const auto lines = stderrBuf_.append(proc_->readAllStandardError());
  for (const auto &l : lines) appendReceiverLine(l);
}

void ReceiverWidget::onFinished(int, QProcess::ExitStatus) {
  if (!proc_) return;
  for (const auto &l : stdoutBuf_.flush()) appendReceiverLine(l);
  for (const auto &l : stderrBuf_.flush()) appendReceiverLine(l);

  flushLogBuffer();
  if (logFile_) {
    logFile_->flush();
    logFile_->close();
    logFile_->deleteLater();
    logFile_ = nullptr;
  }

  flushPendingRows();

  proc_->deleteLater();
  proc_ = nullptr;
  btnStart_->setEnabled(true);
  btnStop_->setEnabled(false);
}

void ReceiverWidget::appendReceiverLine(const QString &line) {
  static const QRegularExpression re(
      R"(^seq=(\d+)\s+len=(\d+)\s+prio=(-?\d+)\s+src=([^:]+):(\d+)\s+(?:expected_)?send_ns=(\d+)\s+rx_ts_ns=(\d+|NA)\s+rx_ts_src=([A-Za-z0-9_]+|NA)\s+latency=([-]?\d+|NA)\s*$)");
  const auto m = re.match(line.trimmed());
  if (!m.hasMatch()) return;

  const QString t = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
  const QStringList cols = QStringList() << t << m.captured(1) << m.captured(2) << m.captured(3) << m.captured(4)
                                        << m.captured(5) << m.captured(6) << m.captured(7) << m.captured(8)
                                        << m.captured(9);

  pendingRows_.enqueue(cols);
  totalMatched_++;
  lastSeq_ = m.captured(1);

  const int maxPending = maxOutputRows_ * 20;
  while (pendingRows_.size() > maxPending) {
    pendingRows_.dequeue();
    droppedUi_++;
  }

  if (logFile_) {
    logWriteBuffer_.append(cols.join('\t').toUtf8());
    logWriteBuffer_.append('\n');
    if (logWriteBuffer_.size() >= 64 * 1024) flushLogBuffer();
  }

  statsDirty_ = true;
}

void ReceiverWidget::addOutputRow(const QStringList &cols) {
  const int row = output_->rowCount();
  output_->insertRow(row);
  for (int c = 0; c < cols.size() && c < output_->columnCount(); ++c) {
    output_->setItem(row, c, new QTableWidgetItem(cols[c]));
  }
  trimOutputRows();
  output_->scrollToBottom();
}

void ReceiverWidget::trimOutputRows() {
  while (output_->rowCount() > maxOutputRows_) output_->removeRow(0);
}

void ReceiverWidget::flushPendingRows() {
  if (!output_) return;

  const int n = qMin(maxRowsPerFlush_, pendingRows_.size());
  if (n <= 0) {
    if (statsDirty_) updateStatsUi();
    return;
  }

  output_->setUpdatesEnabled(false);
  const int startRow = output_->rowCount();
  output_->setRowCount(startRow + n);

  for (int i = 0; i < n; ++i) {
    const QStringList cols = pendingRows_.dequeue();
    for (int c = 0; c < cols.size() && c < output_->columnCount(); ++c) {
      output_->setItem(startRow + i, c, new QTableWidgetItem(cols[c]));
    }
  }

  trimOutputRows();
  output_->setUpdatesEnabled(true);
  output_->scrollToBottom();

  updateStatsUi();
}

void ReceiverWidget::updateStatsUi() {
  if (!stats_) return;
  statsDirty_ = false;

  const QString last = lastSeq_.isEmpty() ? "-" : lastSeq_;
  const int shown = output_ ? output_->rowCount() : 0;
  const int pending = pendingRows_.size();
  stats_->setText(QString("接收: %1  显示(末尾%2): %3  待刷新: %4  UI丢弃: %5  last_seq: %6")
                      .arg(totalMatched_)
                      .arg(maxOutputRows_)
                      .arg(shown)
                      .arg(pending)
                      .arg(droppedUi_)
                      .arg(last));

  if (logPath_) {
    logPath_->setText(logFilePath_.isEmpty() ? "log: (disabled)" : QString("log: %1").arg(logFilePath_));
  }
}

void ReceiverWidget::flushLogBuffer() {
  if (!logFile_ || logWriteBuffer_.isEmpty()) return;
  const qint64 wrote = logFile_->write(logWriteBuffer_);
  if (wrote > 0) logWriteBuffer_.remove(0, (int)wrote);
}
