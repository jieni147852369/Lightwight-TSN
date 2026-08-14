#include "SenderWidget.h"

#include <QButtonGroup>
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
#include <QRadioButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <unistd.h>
#include <time.h>

static QString ifaceLabel(const QNetworkInterface &iface) {
  const bool up = iface.flags().testFlag(QNetworkInterface::IsUp);
  return QString("%1 (%2)").arg(iface.name(), up ? "up" : "down");
}

static quint64 taiNowNs(bool *ok) {
  timespec ts;
  if (clock_gettime(CLOCK_TAI, &ts) != 0) {
    if (ok) *ok = false;
    return 0;
  }
  if (ok) *ok = true;
  return static_cast<quint64>(ts.tv_sec) * 1000000000ULL +
         static_cast<quint64>(ts.tv_nsec);
}

SenderWidget::SenderWidget(QWidget *parent) : QWidget(parent) {
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

  auto *cfgBox = new QGroupBox("发帧配置", this);
  auto *cfgLayout = new QVBoxLayout(cfgBox);

  auto *topForm = new QFormLayout();
  iface_ = new QComboBox(cfgBox);
  iface_->setEditable(true);
  btnReloadIfaces_ = new QPushButton("刷新接口", cfgBox);
  auto *ifaceRow = new QWidget(cfgBox);
  auto *ifaceRowLayout = new QHBoxLayout(ifaceRow);
  ifaceRowLayout->setContentsMargins(0, 0, 0, 0);
  ifaceRowLayout->addWidget(iface_, 1);
  ifaceRowLayout->addWidget(btnReloadIfaces_);
  topForm->addRow("interface", ifaceRow);

  auto *startRow = new QWidget(cfgBox);
  auto *startLayout = new QHBoxLayout(startRow);
  startLayout->setContentsMargins(0, 0, 0, 0);
  rbNow_ = new QRadioButton("立即(0)", startRow);
  rbAbs_ = new QRadioButton("绝对(ns)", startRow);
  rbDelay_ = new QRadioButton("延迟(秒)", startRow);
  absNs_ = new QLineEdit(startRow);
  absNs_->setPlaceholderText("例如：1765647882066464926");
  delaySec_ = new QDoubleSpinBox(startRow);
  delaySec_->setRange(0.0, 3600.0);
  delaySec_->setDecimals(3);
  delaySec_->setSingleStep(0.1);
  delaySec_->setValue(1.0);

  startLayout->addWidget(rbNow_);
  startLayout->addWidget(rbAbs_);
  startLayout->addWidget(absNs_, 1);
  startLayout->addSpacing(8);
  startLayout->addWidget(rbDelay_);
  startLayout->addWidget(delaySec_);
  topForm->addRow("base_start_ns", startRow);

  warmupMs_ = new QSpinBox(cfgBox);
  warmupMs_->setRange(0, 60000);
  warmupMs_->setValue(500);
  warmupMs_->setToolTip("实际发包起点 = base_start_ns + warmup_ms（base_start_ns 为 0 表示 now）");
  auto *warmRow = new QWidget(cfgBox);
  auto *warmLayout = new QHBoxLayout(warmRow);
  warmLayout->setContentsMargins(0, 0, 0, 0);
  warmLayout->addWidget(warmupMs_);
  warmLayout->addWidget(new QLabel("ms", warmRow));
  warmLayout->addStretch(1);
  topForm->addRow("warmup_ms", warmRow);

  cfgLayout->addLayout(topForm);

  auto *bg = new QButtonGroup(this);
  bg->addButton(rbNow_);
  bg->addButton(rbAbs_);
  bg->addButton(rbDelay_);
  rbNow_->setChecked(true);
  absNs_->setEnabled(false);
  delaySec_->setEnabled(false);
  warmupMs_->setEnabled(true);
  connect(rbNow_, &QRadioButton::toggled, this, [this](bool checked) {
    if (!checked) return;
    absNs_->setEnabled(false);
    delaySec_->setEnabled(false);
  });
  connect(rbAbs_, &QRadioButton::toggled, this, [this](bool checked) {
    absNs_->setEnabled(checked);
    if (checked) delaySec_->setEnabled(false);
  });
  connect(rbDelay_, &QRadioButton::toggled, this, [this](bool checked) {
    delaySec_->setEnabled(checked);
    if (checked) absNs_->setEnabled(false);
  });

  frames_ = new QTableWidget(cfgBox);
  frames_->setColumnCount(8);
  frames_->setHorizontalHeaderLabels(QStringList()
                                     << "name"
                                     << "dst_ip"
                                     << "dst_port"
                                     << "udp_src_port"
                                     << "frame_count"
                                     << "payload_len"
                                     << "period_us"
                                     << "socket_priority");
  frames_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  frames_->setSelectionBehavior(QAbstractItemView::SelectRows);
  frames_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  cfgLayout->addWidget(frames_);

  auto *frameBtns = new QHBoxLayout();
  btnAddFrame_ = new QPushButton("新增帧", cfgBox);
  btnRemoveFrame_ = new QPushButton("删除选中帧", cfgBox);
  frameBtns->addWidget(btnAddFrame_);
  frameBtns->addWidget(btnRemoveFrame_);
  frameBtns->addStretch(1);
  cfgLayout->addLayout(frameBtns);

  root->addWidget(cfgBox, 2);

  auto *ctrl = new QHBoxLayout();
  btnStart_ = new QPushButton("启动", this);
  btnStartMulti_ = new QPushButton("多进程启动", this);
  btnStop_ = new QPushButton("停止", this);
  btnClear_ = new QPushButton("清空帧信息", this);
  btnExport_ = new QPushButton("导出TXT", this);
  btnStop_->setEnabled(false);
  ctrl->addWidget(btnStart_);
  ctrl->addWidget(btnStartMulti_);
  ctrl->addWidget(btnStop_);
  ctrl->addStretch(1);
  ctrl->addWidget(btnClear_);
  ctrl->addWidget(btnExport_);
  root->addLayout(ctrl);

  output_ = new QTableWidget(this);
  output_->setColumnCount(7);
  output_->setHorizontalHeaderLabels(QStringList() << "stream"
                                                   << "prio"
                                                   << "seq"
                                                   << "send_ns"
                                                   << "sched_ns"
                                                   << "wake_late_ns"
                                                   << "start_ns");
  output_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  output_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  output_->setSelectionBehavior(QAbstractItemView::SelectRows);
  root->addWidget(output_, 3);

  connect(btnBrowse_, &QPushButton::clicked, this, &SenderWidget::browseConfig);
  connect(btnOpen_, &QPushButton::clicked, this, &SenderWidget::loadConfig);
  connect(btnSave_, &QPushButton::clicked, this, &SenderWidget::saveConfig);
  connect(btnSaveAs_, &QPushButton::clicked, this, &SenderWidget::saveConfigAs);
  connect(btnAddFrame_, &QPushButton::clicked, this, &SenderWidget::addFrame);
  connect(btnRemoveFrame_, &QPushButton::clicked, this, &SenderWidget::removeSelectedFrames);
  connect(btnStart_, &QPushButton::clicked, this, &SenderWidget::startSender);
  connect(btnStartMulti_, &QPushButton::clicked, this, &SenderWidget::startSenderMulti);
  connect(btnStop_, &QPushButton::clicked, this, &SenderWidget::stopSender);
  connect(btnClear_, &QPushButton::clicked, this, &SenderWidget::clearOutput);
  connect(btnExport_, &QPushButton::clicked, this, &SenderWidget::exportOutputTxt);
  connect(btnReloadIfaces_, &QPushButton::clicked, this, &SenderWidget::reloadInterfaces);

  tickTimer_ = new QTimer(this);
  tickTimer_->setInterval(30);
  connect(tickTimer_, &QTimer::timeout, this, &SenderWidget::onTick);
  tickTimer_->start();

  reloadInterfaces();
  loadConfig();
}

QString SenderWidget::resolveDefaultConfigPath() const {
  const QStringList candidates = {
      QDir(QCoreApplication::applicationDirPath()).filePath("../configs/tsn_multi_sender.conf"),
      QDir::current().filePath("configs/tsn_multi_sender.conf"),
      QDir::current().filePath("../configs/tsn_multi_sender.conf"),
  };
  for (const auto &p : candidates) {
    if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
  }
  return QDir::current().filePath("tsn_multi_sender.conf");
}

QString SenderWidget::resolveSenderBinary() const {
  const QStringList candidates = {
      QDir(QCoreApplication::applicationDirPath()).filePath("tsn_multi_sender"),
      QDir::current().filePath("bin/tsn_multi_sender"),
      QDir::current().filePath("../bin/tsn_multi_sender"),
  };
  for (const auto &p : candidates) {
    if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
  }
  return QString();
}

QString SenderWidget::makeLogFilePath(const QString &label) {
  QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  if (dir.isEmpty()) dir = QDir::currentPath();
  QString safe = label.trimmed();
  if (safe.isEmpty()) safe = "run";
  safe.replace(QRegularExpression("[^A-Za-z0-9_.-]"), "_");
  const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmsszzz");
  const quint64 idx = ++logFileCounter_;
  return QDir(dir).filePath(QString("tsn_multi_sender_%1_%2_%3.log").arg(ts).arg(idx).arg(safe));
}

QString SenderWidget::makeTempConfigPath(const QString &label, int index) const {
  QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  if (dir.isEmpty()) dir = QDir::currentPath();
  QString safe = label.trimmed();
  if (safe.isEmpty()) safe = "frame";
  safe.replace(QRegularExpression("[^A-Za-z0-9_.-]"), "_");
  return QDir(dir).filePath(QString("tsn_multi_sender_cfg_%1_%2.conf").arg(index + 1).arg(safe));
}

void SenderWidget::reloadInterfaces() {
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

void SenderWidget::browseConfig() {
  const QString file = QFileDialog::getOpenFileName(
      this, "选择发帧配置文件", QFileInfo(configPath_->text()).absolutePath(),
      "Config (*.conf *.txt);;All Files (*)");
  if (file.isEmpty()) return;
  configPath_->setText(file);
  loadConfig();
}

bool SenderWidget::fillUiFromConfig(const SenderConfig &cfg, QString *) {
  const int idx = iface_->findData(cfg.interface);
  if (idx >= 0) iface_->setCurrentIndex(idx);

  if (warmupMs_) warmupMs_->setValue(static_cast<int>(cfg.warmupMs));

  if (cfg.baseStartNs == 0) {
    rbNow_->setChecked(true);
    absNs_->setText(QString());
  } else {
    rbAbs_->setChecked(true);
    absNs_->setText(QString::number(cfg.baseStartNs));
  }

  frames_->setRowCount(0);
  for (const auto &fr : cfg.frames) {
    const int row = frames_->rowCount();
    frames_->insertRow(row);
    frames_->setItem(row, 0, new QTableWidgetItem(fr.name));
    frames_->setItem(row, 1, new QTableWidgetItem(fr.dstIp));
    frames_->setItem(row, 2, new QTableWidgetItem(QString::number(fr.dstPort)));
    frames_->setItem(row, 3, new QTableWidgetItem(fr.udpSrcPort ? QString::number(*fr.udpSrcPort) : QString()));
    frames_->setItem(row, 4, new QTableWidgetItem(fr.frameCount ? QString::number(*fr.frameCount) : QString()));
    frames_->setItem(row, 5, new QTableWidgetItem(QString::number(fr.payloadLen)));
    frames_->setItem(row, 6, new QTableWidgetItem(QString::number(fr.periodUs)));
    auto *prio = new QComboBox(frames_);
    for (int p = 0; p <= 7; ++p) prio->addItem(QString::number(p), p);
    const int val = qBound(0, fr.socketPriority, 7);
    prio->setCurrentIndex(val);
    frames_->setCellWidget(row, 7, prio);
  }
  if (frames_->rowCount() == 0) addFrame();
  return true;
}

void SenderWidget::loadConfig() {
  QString error;
  SenderConfig cfg;
  if (!SenderConfigIo::loadFromFile(configPath_->text().trimmed(), &cfg, &error)) {
    QMessageBox::warning(this, "打开失败", error);
    if (frames_->rowCount() == 0) addFrame();
    return;
  }
  fillUiFromConfig(cfg, &error);
}

SenderConfig SenderWidget::configFromUi(QString *error, bool computeDelayToAbsolute) const {
  SenderConfig cfg;
  cfg.interface = iface_->currentData().toString().trimmed();
  if (cfg.interface.isEmpty()) cfg.interface = iface_->currentText().trimmed();
  cfg.warmupMs = warmupMs_ ? static_cast<quint64>(warmupMs_->value()) : 500;

  StartMode mode = StartMode::Now;
  if (rbAbs_->isChecked()) mode = StartMode::AbsoluteNs;
  if (rbDelay_->isChecked()) mode = StartMode::DelaySeconds;

  if (mode == StartMode::Now) {
    cfg.baseStartNs = 0;
  } else if (mode == StartMode::AbsoluteNs) {
    bool ok = false;
    cfg.baseStartNs = absNs_->text().trimmed().toULongLong(&ok, 0);
    if (!ok || cfg.baseStartNs == 0) {
      if (error) *error = "绝对 base_start_ns 无效";
      return {};
    }
  } else {
    if (!computeDelayToAbsolute) {
      cfg.baseStartNs = 0;
    } else {
      bool ok = false;
      const quint64 now = taiNowNs(&ok);
      if (!ok || now == 0) {
        if (error) *error = "无法获取 CLOCK_TAI 当前时间（clock_gettime(CLOCK_TAI) 失败）";
        return {};
      }
      const double sec = delaySec_->value();
      const quint64 add = static_cast<quint64>(sec * 1000000000.0);
      cfg.baseStartNs = now + add;
    }
  }

  const int rows = frames_->rowCount();
  cfg.frames.clear();
  cfg.frames.reserve(static_cast<size_t>(rows));
  for (int r = 0; r < rows; ++r) {
    auto get = [this, r](int c) -> QString {
      auto *it = frames_->item(r, c);
      return it ? it->text().trimmed() : QString();
    };
    SenderFrame fr;
    fr.name = get(0);
    if (fr.name.isEmpty()) fr.name = QString("stream%1").arg(r + 1);
    fr.dstIp = get(1);
    fr.dstPort = get(2).toInt();
    const QString udpSrc = get(3);
    if (!udpSrc.isEmpty()) fr.udpSrcPort = udpSrc.toInt();
    const QString cnt = get(4);
    if (!cnt.isEmpty()) fr.frameCount = cnt.toULongLong();
    fr.payloadLen = get(5).toInt();
    fr.periodUs = get(6).toULongLong();
    int prioVal = -1;
    if (auto *w = frames_->cellWidget(r, 7)) {
      if (auto *cb = qobject_cast<QComboBox *>(w)) {
        prioVal = cb->currentData().toInt();
      }
    }
    if (prioVal < 0) prioVal = get(7).toInt();
    fr.socketPriority = prioVal;
    cfg.frames.push_back(fr);
  }

  if (cfg.interface.isEmpty()) {
    if (error) *error = "interface 不能为空";
    return {};
  }
  if (cfg.frames.empty()) {
    if (error) *error = "至少需要一个 frame";
    return {};
  }
  for (const auto &fr : cfg.frames) {
    if (fr.dstIp.isEmpty() || fr.dstPort <= 0 || fr.dstPort > 65535 ||
        fr.payloadLen <= 0 || fr.periodUs == 0) {
      if (error) *error = QString("frame [%1] 配置不完整").arg(fr.name);
      return {};
    }
    if (fr.socketPriority < 0 || fr.socketPriority > 7) {
      if (error) *error = QString("frame [%1] socket_priority 应为 0..7").arg(fr.name);
      return {};
    }
  }

  return cfg;
}

void SenderWidget::saveConfig() {
  QString error;
  const auto cfg = configFromUi(&error, /*computeDelayToAbsolute=*/true);
  if (!error.isEmpty()) {
    QMessageBox::warning(this, "保存失败", error);
    return;
  }
  if (!SenderConfigIo::saveToFile(configPath_->text().trimmed(), cfg, &error)) {
    QMessageBox::warning(this, "保存失败", error);
    return;
  }
}

void SenderWidget::saveConfigAs() {
  const QString file = QFileDialog::getSaveFileName(
      this, "另存为发帧配置", QFileInfo(configPath_->text()).absolutePath(),
      "Config (*.conf *.txt);;All Files (*)");
  if (file.isEmpty()) return;
  configPath_->setText(file);
  saveConfig();
}

void SenderWidget::addFrame() {
  const int row = frames_->rowCount();
  frames_->insertRow(row);
  frames_->setItem(row, 0, new QTableWidgetItem(QString("stream%1").arg(row + 1)));
  frames_->setItem(row, 1, new QTableWidgetItem("192.168.150.2"));
  frames_->setItem(row, 2, new QTableWidgetItem("5000"));
  frames_->setItem(row, 3, new QTableWidgetItem(QString()));
  frames_->setItem(row, 4, new QTableWidgetItem(QString()));
  frames_->setItem(row, 5, new QTableWidgetItem("128"));
  frames_->setItem(row, 6, new QTableWidgetItem("100"));
  auto *prio = new QComboBox(frames_);
  for (int p = 0; p <= 7; ++p) prio->addItem(QString::number(p), p);
  prio->setCurrentIndex(7);
  frames_->setCellWidget(row, 7, prio);
}

void SenderWidget::removeSelectedFrames() {
  QList<int> rows;
  for (auto *it : frames_->selectedItems()) rows.push_back(it->row());
  std::sort(rows.begin(), rows.end());
  rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
  for (int i = rows.size() - 1; i >= 0; --i) frames_->removeRow(rows[i]);
  if (frames_->rowCount() == 0) addFrame();
}

bool SenderWidget::ensureReadyToStart(QString *error) const {
  if (hasRunningProcess()) {
    if (error) *error = "running";
    return false;
  }
  if (geteuid() != 0) {
    if (error) *error = "root";
    return false;
  }
  return true;
}

void SenderWidget::cleanupContext(SenderProcessContext *ctx) {
  if (!ctx) return;
  if (ctx->proc) {
    ctx->proc->deleteLater();
    ctx->proc = nullptr;
  }
  if (ctx->liveLogWriter) {
    ctx->liveLogWriter->close();
    ctx->liveLogWriter->deleteLater();
    ctx->liveLogWriter = nullptr;
  }
  if (ctx->deleteConfigOnExit && !ctx->configPath.isEmpty()) {
    QFile::remove(ctx->configPath);
  }
}

void SenderWidget::resetForNewRun() {
  for (auto &ctx : processes_) {
    cleanupContext(ctx.get());
  }
  processes_.clear();
  lastLogFiles_.clear();
  clearOutput();
}

bool SenderWidget::startSenderProcess(const QString &bin, const QString &configPath, const QString &label,
                                      bool deleteConfigOnExit, QString *error) {
  auto ctx = std::make_unique<SenderProcessContext>();
  ctx->name = label;
  ctx->configPath = configPath;
  ctx->deleteConfigOnExit = deleteConfigOnExit;
  ctx->logFilePath = makeLogFilePath(label);
  auto *ctxRaw = ctx.get();
  if (!ctx->logFilePath.isEmpty()) {
    ctx->liveLogWriter = new QFile(ctx->logFilePath, this);
    if (!ctx->liveLogWriter->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      if (error) *error = "无法创建日志文件。";
      cleanupContext(ctxRaw);
      return false;
    }
  }

  QProcess *proc = new QProcess(this);
  ctx->proc = proc;

  QString program = bin;
  QStringList args = QStringList() << configPath;
  const QString stdbuf = QStandardPaths::findExecutable("stdbuf");
  if (!stdbuf.isEmpty()) {
    program = stdbuf;
    args = QStringList() << "-oL"
                         << "-eL" << bin << configPath;
  }
  proc->setProgram(program);
  proc->setArguments(args);
  proc->setProcessChannelMode(QProcess::SeparateChannels);
  connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &SenderWidget::onFinished);
  connect(proc, &QProcess::readyReadStandardOutput, this, [this, ctxRaw]() { handleProcessOutput(ctxRaw, true); });
  connect(proc, &QProcess::readyReadStandardError, this, [this, ctxRaw]() { handleProcessOutput(ctxRaw, false); });

  processes_.push_back(std::move(ctx));
  proc->start();
  if (!proc->waitForStarted(3000)) {
    if (error) *error = "无法启动发帧进程。";
    cleanupContext(ctxRaw);
    processes_.erase(std::remove_if(processes_.begin(), processes_.end(),
                                    [ctxRaw](const auto &p) { return p.get() == ctxRaw; }),
                     processes_.end());
    return false;
  }

  return true;
}

SenderWidget::SenderProcessContext *SenderWidget::findContext(QProcess *proc) {
  if (!proc) return nullptr;
  for (auto &ctx : processes_) {
    if (ctx->proc == proc) return ctx.get();
  }
  return nullptr;
}

bool SenderWidget::hasRunningProcess() const { return !processes_.empty(); }

void SenderWidget::updateButtons() {
  const bool running = hasRunningProcess();
  if (btnStart_) btnStart_->setEnabled(!running);
  if (btnStartMulti_) btnStartMulti_->setEnabled(!running);
  if (btnStop_) btnStop_->setEnabled(running);
}

void SenderWidget::handleProcessOutput(SenderProcessContext *ctx, bool isStdout) {
  if (!ctx || !ctx->proc) return;
  const QByteArray data =
      isStdout ? ctx->proc->readAllStandardOutput() : ctx->proc->readAllStandardError();
  if (data.isEmpty()) return;

  if (ctx->liveLogWriter && ctx->liveLogWriter->isOpen()) {
    ctx->liveLogWriter->write(data);
    ctx->liveLogWriter->flush();
  }

  const auto lines = ctx->logBuf.append(data);
  for (const auto &l : lines) appendSenderLine(l);
}

void SenderWidget::startSender() {
  QString prepError;
  if (!ensureReadyToStart(&prepError)) {
    if (prepError == "running") {
      QMessageBox::information(this, "提示", "发帧程序已在运行。");
    } else {
      QMessageBox::warning(this, "权限不足", "建议用 sudo 启动 TSN_SoftWare 后再启动发帧。");
    }
    return;
  }

  QString error;
  const auto cfg = configFromUi(&error, /*computeDelayToAbsolute=*/true);
  if (!error.isEmpty()) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }
  if (!SenderConfigIo::saveToFile(configPath_->text().trimmed(), cfg, &error)) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }

  const QString bin = resolveSenderBinary();
  if (bin.isEmpty()) {
    QMessageBox::warning(this, "启动失败",
                         "找不到发帧程序：bin/tsn_multi_sender（请先在 TSN_SoftWare 目录 qmake+make 生成）");
    return;
  }

  resetForNewRun();
  if (!startSenderProcess(bin, configPath_->text().trimmed(), "all_frames", /*deleteConfigOnExit=*/false,
                          &error)) {
    if (error.isEmpty()) error = "无法启动发帧进程。";
    QMessageBox::warning(this, "启动失败", error);
    return;
  }

  lastLogFiles_ = QStringList() << processes_.back()->logFilePath;
  updateButtons();
}

void SenderWidget::startSenderMulti() {
  QString prepError;
  if (!ensureReadyToStart(&prepError)) {
    if (prepError == "running") {
      QMessageBox::information(this, "提示", "发帧程序已在运行。");
    } else {
      QMessageBox::warning(this, "权限不足", "建议用 sudo 启动 TSN_SoftWare 后再启动发帧。");
    }
    return;
  }

  QString error;
  const auto cfg = configFromUi(&error, /*computeDelayToAbsolute=*/true);
  if (!error.isEmpty()) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }
  if (!SenderConfigIo::saveToFile(configPath_->text().trimmed(), cfg, &error)) {
    QMessageBox::warning(this, "启动失败", error);
    return;
  }

  const QString bin = resolveSenderBinary();
  if (bin.isEmpty()) {
    QMessageBox::warning(this, "启动失败",
                         "找不到发帧程序：bin/tsn_multi_sender（请先在 TSN_SoftWare 目录 qmake+make 生成）");
    return;
  }

  resetForNewRun();

  QStringList logFiles;
  bool allOk = true;
  int idx = 0;
  for (const auto &fr : cfg.frames) {
    SenderConfig singleCfg;
    singleCfg.interface = cfg.interface;
    singleCfg.baseStartNs = cfg.baseStartNs;
    singleCfg.warmupMs = cfg.warmupMs;
    singleCfg.frames = {fr};

    const QString tmpCfg = makeTempConfigPath(fr.name, idx);
    QString cfgErr;
    if (!SenderConfigIo::saveToFile(tmpCfg, singleCfg, &cfgErr)) {
      error = cfgErr;
      allOk = false;
      break;
    }

    if (!startSenderProcess(bin, tmpCfg, fr.name, /*deleteConfigOnExit=*/true, &error)) {
      allOk = false;
      break;
    }

    logFiles << processes_.back()->logFilePath;
    ++idx;
  }

  if (!allOk) {
    if (hasRunningProcess()) stopSender();
    if (error.isEmpty()) error = "无法启动发帧进程。";
    QMessageBox::warning(this, "启动失败", error);
    return;
  }

  lastLogFiles_ = logFiles;
  updateButtons();
}

void SenderWidget::stopSender() {
  if (processes_.empty()) return;

  std::vector<SenderProcessContext *> contexts;
  contexts.reserve(processes_.size());
  for (auto &ctx : processes_) contexts.push_back(ctx.get());

  for (auto *ctx : contexts) {
    if (ctx && ctx->proc) disconnect(ctx->proc, nullptr, this, nullptr);
  }

  for (auto *ctx : contexts) {
    if (ctx && ctx->proc) ctx->proc->terminate();
  }
  for (auto *ctx : contexts) {
    if (!ctx || !ctx->proc) continue;
    if (!ctx->proc->waitForFinished(1500)) ctx->proc->kill();
    ctx->proc->waitForFinished(500);
  }

  for (auto *ctx : contexts) {
    handleProcessOutput(ctx, true);
    handleProcessOutput(ctx, false);
  }
  flushPendingRows();

  for (auto &ctx : processes_) cleanupContext(ctx.get());
  processes_.clear();
  updateButtons();
}

void SenderWidget::clearOutput() {
  output_->setRowCount(0);
  pendingRows_.clear();
  droppedUi_ = 0;
}

void SenderWidget::exportOutputTxt() {
  const QString file = QFileDialog::getSaveFileName(
      this, "导出发帧信息 TXT", QDir::currentPath(), "Text (*.txt);;All Files (*)");
  if (file.isEmpty()) return;

  QFile f(file);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QMessageBox::warning(this, "导出失败", "无法写入文件。");
    return;
  }
  QTextStream out(&f);
  const QStringList headers = QStringList() << "stream"
                                            << "prio"
                                            << "seq"
                                            << "send_ns"
                                            << "sched_ns"
                                            << "wake_late_ns"
                                            << "start_ns";
  out << headers.join('\t') << "\n";

  const auto writeLine = [&](const QString &line) {
    const QString trimmed = line.trimmed();
    static const QRegularExpression re(
        R"(^\[(?<stream>[^\]]+)\]\s+frame\s+(?<seq>\d+):\s+prio=(?<prio>\d+)\s+start_ns=(?<start>\d+)\s+sched_ns=(?<sched>\d+)\s+wake_late_ns=(?<late>-?\d+)\s+send_ns=(?<send>\d+)\s*$)");
    const auto m = re.match(trimmed);
    if (!m.hasMatch()) return;
    out << (QStringList() << m.captured("stream") << m.captured("prio") << m.captured("seq")
                          << m.captured("send") << m.captured("sched") << m.captured("late")
                          << m.captured("start"))
               .join('\t')
        << "\n";
  };

  bool wroteFromLogs = false;
  for (const auto &logPath : lastLogFiles_) {
    if (logPath.isEmpty() || !QFileInfo::exists(logPath)) continue;
    QFile lf(logPath);
    if (!lf.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
    ProcessLineBuffer buf;
    while (!lf.atEnd()) {
      const QByteArray chunk = lf.read(64 * 1024);
      const auto lines = buf.append(chunk);
      for (const auto &l : lines) writeLine(l);
    }
    for (const auto &l : buf.flush()) writeLine(l);
    wroteFromLogs = true;
  }
  if (wroteFromLogs) return;

  // Fallback: export whatever is currently shown.
  for (int r = 0; r < output_->rowCount(); ++r) {
    QStringList cols;
    for (int c = 0; c < output_->columnCount(); ++c) {
      auto *it = output_->item(r, c);
      cols << (it ? it->text() : "");
    }
    out << cols.join('\t') << "\n";
  }
}

void SenderWidget::onTick() {
  flushPendingRows();
}

void SenderWidget::onFinished(int, QProcess::ExitStatus) {
  auto *proc = qobject_cast<QProcess *>(sender());
  auto *ctx = findContext(proc);
  if (!ctx) return;

  handleProcessOutput(ctx, true);
  handleProcessOutput(ctx, false);
  flushPendingRows();
  cleanupContext(ctx);
  processes_.erase(std::remove_if(processes_.begin(), processes_.end(),
                                  [ctx](const auto &p) { return p.get() == ctx; }),
                   processes_.end());

  updateButtons();
}

void SenderWidget::appendSenderLine(const QString &line) {
  static const QRegularExpression re(
      R"(^\[(?<stream>[^\]]+)\]\s+frame\s+(?<seq>\d+):\s+prio=(?<prio>\d+)\s+start_ns=(?<start>\d+)\s+sched_ns=(?<sched>\d+)\s+wake_late_ns=(?<late>-?\d+)\s+send_ns=(?<send>\d+)\s*$)");
  const auto m = re.match(line.trimmed());
  if (!m.hasMatch()) return;

  pendingRows_.enqueue(QStringList() << m.captured("stream") << m.captured("prio") << m.captured("seq")
                                     << m.captured("send") << m.captured("sched") << m.captured("late")
                                     << m.captured("start"));
  const int maxPending = maxOutputRows_ * 20;
  while (pendingRows_.size() > maxPending) {
    pendingRows_.dequeue();
    droppedUi_++;
  }
}

void SenderWidget::flushPendingRows() {
  if (!output_) return;

  const int n = qMin(maxRowsPerFlush_, pendingRows_.size());
  if (n <= 0) return;

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
}

void SenderWidget::trimOutputRows() {
  if (!output_) return;
  const int extra = output_->rowCount() - maxOutputRows_;
  if (extra <= 0) return;
  if (auto *m = output_->model()) {
    m->removeRows(0, extra);
  } else {
    for (int i = 0; i < extra; ++i) output_->removeRow(0);
  }
}

void SenderWidget::updateVlanDerived() {}
