#include "ConfigTab.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QDateTime>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStringList>
#include <QStackedWidget>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QAbstractItemView>
#include <QTextStream>
#include <QVBoxLayout>
#include <QList>

#include <fcntl.h>
#include <errno.h>
#include <linux/ptp_clock.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#include <unistd.h>

#ifndef FD_TO_CLOCKID
#define FD_TO_CLOCKID(fd) ((((clockid_t) ~((fd))) << 3) | 3)
#endif

static QString findTimesyncBinary(const QString &name) {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      QDir(appDir).filePath(name),
      QDir(appDir).filePath(QString("../bin/%1").arg(name)),
      QDir(appDir).filePath(QString("../%1").arg(name)),
      QDir(appDir).filePath(QString("../vendor/timesync-core/%1").arg(name)),
  };
  for (const auto &path : candidates) {
    QFileInfo info(path);
    if (info.exists() && info.isFile() && info.isExecutable()) {
      return info.absoluteFilePath();
    }
  }
  return name;
}

static QString ifaceLabel(const QNetworkInterface &iface) {
  const bool up = iface.flags().testFlag(QNetworkInterface::IsUp);
  return QString("%1 (%2, %3)")
      .arg(iface.name(), up ? "up" : "down", iface.hardwareAddress());
}

static QFrame *makeHLine(QWidget *parent) {
  auto *line = new QFrame(parent);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  return line;
}

static bool isInterfaceUp(const QString &ifaceName, QString *error) {
  const QNetworkInterface iface = QNetworkInterface::interfaceFromName(ifaceName);
  if (!iface.isValid()) {
    if (error) *error = QString("接口 %1 不存在").arg(ifaceName);
    return false;
  }
  const bool up = iface.flags().testFlag(QNetworkInterface::IsUp) &&
                  iface.flags().testFlag(QNetworkInterface::IsRunning);
  if (!up) {
    if (error) *error =
        QString("接口 %1 当前为 down 状态，请先启用后再执行。").arg(ifaceName);
    return false;
  }
  return true;
}

static qint64 timespecToNs(const struct timespec &ts) {
  return static_cast<qint64>(ts.tv_sec) * 1000000000LL +
         static_cast<qint64>(ts.tv_nsec);
}

static struct timespec nsToTimespec(qint64 ns) {
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(ns / 1000000000LL);
  ts.tv_nsec = static_cast<long>(ns % 1000000000LL);
  if (ts.tv_nsec < 0) {
    ts.tv_nsec += 1000000000L;
    ts.tv_sec -= 1;
  }
  return ts;
}

static QString guessDefaultPhcDevice() {
  QDir dev("/dev");
  const QStringList ptps =
      dev.entryList(QStringList() << "ptp*", QDir::System | QDir::Files, QDir::Name);
  if (!ptps.isEmpty()) {
    return dev.filePath(ptps.first());
  }
  return QString("/dev/ptp0");
}

static bool readPhcTimeNs(const QString &device, qint64 *ns, QString *error) {
  if (ns) *ns = 0;
  QByteArray path = device.toLocal8Bit();
  int fd = ::open(path.constData(), O_RDONLY);
  if (fd < 0) {
    if (error)
      *error = QString("无法打开 PHC 设备 %1：%2")
                   .arg(device, QString::fromLocal8Bit(strerror(errno)));
    return false;
  }

  struct ptp_clock_caps caps;
  if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps) != 0) {
    if (error)
      *error = QString("设备 %1 不是有效的 PHC：%2")
                   .arg(device, QString::fromLocal8Bit(strerror(errno)));
    close(fd);
    return false;
  }

  const clockid_t clkid = FD_TO_CLOCKID(fd);
  struct timespec ts;
  if (clock_gettime(clkid, &ts) != 0) {
    if (error)
      *error = QString("读取 PHC 时间失败：%1")
                   .arg(QString::fromLocal8Bit(strerror(errno)));
    close(fd);
    return false;
  }
  close(fd);
  if (ns) *ns = timespecToNs(ts);
  return true;
}

static bool writePhcTimeNs(const QString &device, qint64 ns, QString *error) {
  const QByteArray path = device.toLocal8Bit();
  int fd = ::open(path.constData(), O_RDWR);
  if (fd < 0) {
    if (error)
      *error = QString("无法打开 PHC 设备 %1：%2")
                   .arg(device, QString::fromLocal8Bit(strerror(errno)));
    return false;
  }

  struct ptp_clock_caps caps;
  if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps) != 0) {
    if (error)
      *error = QString("设备 %1 不是有效的 PHC：%2")
                   .arg(device, QString::fromLocal8Bit(strerror(errno)));
    close(fd);
    return false;
  }

  const clockid_t clkid = FD_TO_CLOCKID(fd);
  const struct timespec ts = nsToTimespec(ns);
  if (clock_settime(clkid, &ts) != 0) {
    if (error)
      *error = QString("设置 PHC 时间失败：%1")
                   .arg(QString::fromLocal8Bit(strerror(errno)));
    close(fd);
    return false;
  }
  close(fd);
  return true;
}

ConfigTab::ConfigTab(QWidget *parent) : QWidget(parent) {
  auto *root = new QVBoxLayout(this);

  // --- 初始化配置 ---
  auto *initBox = new QGroupBox("初始化配置", this);
  auto *initLayout = new QVBoxLayout(initBox);
  auto *form = new QFormLayout();

  phyIface_ = new QComboBox(initBox);
  btnIfaces_ = new QPushButton("刷新接口", initBox);
  auto *ifaceRow = new QWidget(initBox);
  auto *ifaceLayout = new QHBoxLayout(ifaceRow);
  ifaceLayout->setContentsMargins(0, 0, 0, 0);
  ifaceLayout->addWidget(phyIface_, 1);
  ifaceLayout->addWidget(btnIfaces_);
  form->addRow("物理端口", ifaceRow);

  phyIpCidr_ = new QLineEdit(initBox);
  form->addRow("物理ip", phyIpCidr_);

  vlanId_ = new QSpinBox(initBox);
  vlanId_->setRange(1, 4094);
  form->addRow("vlan号", vlanId_);

  vlanIface_ = new QLineEdit(initBox);
  form->addRow("vlan端口", vlanIface_);

  vlanIpCidr_ = new QLineEdit(initBox);
  form->addRow("vlan ip", vlanIpCidr_);

  initLayout->addLayout(form);

  auto *initActions = new QHBoxLayout();
  btnInitApply_ = new QPushButton("确定", initBox);
  initActions->addStretch(1);
  initActions->addWidget(btnInitApply_);
  initLayout->addLayout(initActions);

  root->addWidget(initBox);
  root->addWidget(makeHLine(this));

  // --- 时间同步配置（占位）---
  auto *timeBox = new QGroupBox("时间同步配置", this);
  auto *timeBoxLayout = new QVBoxLayout(timeBox);
  auto *timeLayout = new QHBoxLayout();
  auto *roleLayout = new QHBoxLayout();
  btnRoleMaster_ = new QPushButton("主节点", timeBox);
  btnRoleBackup_ = new QPushButton("备用节点", timeBox);
  btnRoleSlave_ = new QPushButton("从节点", timeBox);
  btnRoleMaster_->setCheckable(true);
  btnRoleBackup_->setCheckable(true);
  btnRoleSlave_->setCheckable(true);
  btnRoleBackup_->setChecked(true);
  roleLayout->addWidget(btnRoleMaster_);
  roleLayout->addWidget(btnRoleBackup_);
  roleLayout->addWidget(btnRoleSlave_);
  roleLayout->addStretch(1);
  auto *roleWidget = new QWidget(timeBox);
  roleWidget->setLayout(roleLayout);
  roleLayout->setContentsMargins(0, 0, 0, 0);
  btnStartTimeSync_ = new QPushButton("开启时间同步", timeBox);
  btnSwitchRole_ = new QPushButton("切换角色", timeBox);
  btnStopTimeSync_ = new QPushButton("停止时间同步", timeBox);
  syncAccuracyLabel_ = new QLabel("同步精度：--", timeBox);
  syncAccuracyLabel_->setVisible(false);
  systemSyncAccuracyLabel_ = new QLabel("系统同步精度：--", timeBox);
  systemSyncAccuracyLabel_->setVisible(false);
  timeLayout->addWidget(roleWidget, 1);
  timeLayout->addStretch(1);
  timeLayout->addWidget(syncAccuracyLabel_);
  timeLayout->addWidget(systemSyncAccuracyLabel_);
  timeLayout->addStretch(1);
  timeLayout->addWidget(btnStartTimeSync_);
  timeLayout->addWidget(btnSwitchRole_);
  timeLayout->addWidget(btnStopTimeSync_);
  timeBoxLayout->addLayout(timeLayout);

  auto *phcForm = new QFormLayout();
  phcDevicePath_ = guessDefaultPhcDevice();
  auto *phcRow = new QHBoxLayout();
  phcTimeLabel_ = new QLabel("PHC 时间：--", timeBox);
  phcTimeLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  btnRefreshPhcTime_ = new QPushButton("刷新 PHC 时间", timeBox);
  phcRow->addWidget(phcTimeLabel_);
  phcRow->addStretch(1);
  phcRow->addWidget(btnRefreshPhcTime_);
  phcForm->addRow("当前时间", phcRow);

  phcAdjustSecondsEdit_ = new QLineEdit(timeBox);
  phcAdjustSecondsEdit_->setPlaceholderText("输入调整秒数，如 5 或 -3");
  phcAdjustSecondsEdit_->setValidator(
      new QRegularExpressionValidator(QRegularExpression("-?\\d+"), phcAdjustSecondsEdit_));
  phcForm->addRow("调整（秒）", phcAdjustSecondsEdit_);

  auto *phcButtons = new QHBoxLayout();
  btnApplyPhcAdjust_ = new QPushButton("应用调整", timeBox);
  phcButtons->addWidget(btnApplyPhcAdjust_);
  phcButtons->addStretch(1);
  phcForm->addRow("", phcButtons);

  timeBoxLayout->addLayout(phcForm);
  root->addWidget(timeBox);
  root->addWidget(makeHLine(this));

  // --- 整形配置 ---
  auto *shapeBox = new QGroupBox("整形配置", this);
  auto *shapeLayout = new QVBoxLayout(shapeBox);

  auto *modeRow = new QHBoxLayout();
  auto *modeLabel = new QLabel("模式", shapeBox);
  cbsRadio_ = new QRadioButton("CBS", shapeBox);
  tasRadio_ = new QRadioButton("TAS", shapeBox);
  cbsRadio_->setChecked(true);
  modeRow->addWidget(modeLabel);
  modeRow->addWidget(cbsRadio_);
  modeRow->addWidget(tasRadio_);
  modeRow->addStretch(1);
  shapeLayout->addLayout(modeRow);

  shapeStack_ = new QStackedWidget(shapeBox);
  shapeStack_->addWidget(buildCbsPage(shapeBox));
  shapeStack_->addWidget(buildTasPage(shapeBox));
  shapeLayout->addWidget(shapeStack_);

  auto *shapeActions = new QHBoxLayout();
  btnShapeApply_ = new QPushButton("确定", shapeBox);
  shapeActions->addStretch(1);
  shapeActions->addWidget(btnShapeApply_);
  shapeLayout->addLayout(shapeActions);

  root->addWidget(shapeBox);

  auto *schemeActions = new QHBoxLayout();
  btnSaveScheme_ = new QPushButton("保存方案", this);
  btnLoadScheme_ = new QPushButton("导入方案", this);
  schemeActions->addStretch(1);
  schemeActions->addWidget(btnSaveScheme_);
  schemeActions->addWidget(btnLoadScheme_);
  root->addLayout(schemeActions);

  root->addStretch(1);

  connect(btnIfaces_, &QPushButton::clicked, this, &ConfigTab::reloadInterfaces);
  connect(phyIface_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ConfigTab::updateDerived);
  connect(vlanId_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ConfigTab::updateDerived);
  connect(vlanIface_, &QLineEdit::textEdited, this,
          [this]() { vlanIfaceManual_ = true; });

  connect(btnInitApply_, &QPushButton::clicked, this, &ConfigTab::applyInitConfig);
  connect(btnStartTimeSync_, &QPushButton::clicked, this, &ConfigTab::startTimeSync);
  connect(btnSwitchRole_, &QPushButton::clicked, this, &ConfigTab::switchRole);
  connect(btnStopTimeSync_, &QPushButton::clicked, this, &ConfigTab::stopTimeSync);
  connect(btnSaveScheme_, &QPushButton::clicked, this, &ConfigTab::saveScheme);
  connect(btnLoadScheme_, &QPushButton::clicked, this, &ConfigTab::loadScheme);
  connect(btnShapeApply_, &QPushButton::clicked, this, &ConfigTab::applyShapingConfig);
  connect(cbsRadio_, &QRadioButton::toggled, this, &ConfigTab::onShapingModeChanged);
  connect(tasRadio_, &QRadioButton::toggled, this, &ConfigTab::onShapingModeChanged);
  connect(btnRoleMaster_, &QPushButton::clicked, this, [this]() {
    btnRoleMaster_->setChecked(true);
    btnRoleBackup_->setChecked(false);
    btnRoleSlave_->setChecked(false);
  });
  connect(btnRoleBackup_, &QPushButton::clicked, this, [this]() {
    btnRoleMaster_->setChecked(false);
    btnRoleBackup_->setChecked(true);
    btnRoleSlave_->setChecked(false);
  });
  connect(btnRoleSlave_, &QPushButton::clicked, this, [this]() {
    btnRoleMaster_->setChecked(false);
    btnRoleBackup_->setChecked(false);
    btnRoleSlave_->setChecked(true);
  });
  connect(btnRoleMaster_, &QPushButton::toggled, this, &ConfigTab::handleRoleToggled);
  connect(btnRoleBackup_, &QPushButton::toggled, this, &ConfigTab::handleRoleToggled);
  connect(btnRoleSlave_, &QPushButton::toggled, this, &ConfigTab::handleRoleToggled);
  connect(btnRefreshPhcTime_, &QPushButton::clicked, this, &ConfigTab::refreshPhcTime);
  connect(btnApplyPhcAdjust_, &QPushButton::clicked, this, &ConfigTab::applyPhcTime);

  reloadInterfaces();
  phyIpCidr_->setText("192.168.50.2/24");
  vlanId_->setValue(100);
  vlanIpCidr_->setText("192.168.150.2/24");
  updateDerived();
  updateSyncAccuracyVisibility();
  if (btnRoleBackup_) currentRole_ = "backup";
  updateStartButtonState();
  refreshPhcTimeInternal(false);
  phcRefreshTimer_ = new QTimer(this);
  phcRefreshTimer_->setInterval(1000);
  connect(phcRefreshTimer_, &QTimer::timeout, this,
          [this]() { refreshPhcTimeInternal(false); });
  phcRefreshTimer_->start();
}

void ConfigTab::reloadInterfaces() {
  const QString prevPhy = phyIface_ ? phyIface_->currentData().toString() : QString();
  if (phyIface_) phyIface_->clear();

  for (const auto &iface : QNetworkInterface::allInterfaces()) {
    if (iface.name() == "lo") continue;
    const QString label = ifaceLabel(iface);
    if (phyIface_) phyIface_->addItem(label, iface.name());
  }

  const auto pickIndex = [](QComboBox *box, const QString &prev) {
    if (!box) return;
    int idx = prev.isEmpty() ? 0 : box->findData(prev);
    if (idx < 0) idx = 0;
    box->setCurrentIndex(idx);
  };

  pickIndex(phyIface_, prevPhy);
  updateDerived();
}

void ConfigTab::updateDerived() {
  if (!vlanIfaceManual_) {
    const QString phy = phyIface_->currentData().toString().trimmed();
    vlanIface_->setText(QString("%1.%2").arg(phy).arg(vlanId_->value()));
  }
}

ConfigTab::InitModel ConfigTab::initModel() const {
  InitModel m;
  m.phyIface = phyIface_->currentData().toString().trimmed();
  m.phyIpCidr = phyIpCidr_->text().trimmed();
  m.vlanId = vlanId_->value();
  m.vlanIface = vlanIface_->text().trimmed();
  m.vlanIpCidr = vlanIpCidr_->text().trimmed();
  return m;
}

bool ConfigTab::validateInit(QString *error) const {
  const auto m = initModel();
  if (m.phyIface.isEmpty()) {
    if (error) *error = "phy_iface 不能为空";
    return false;
  }
  if (m.phyIpCidr.isEmpty()) {
    if (error) *error = "phy_ip_cidr 不能为空";
    return false;
  }
  if (m.vlanIface.isEmpty()) {
    if (error) *error = "vlan_iface 不能为空";
    return false;
  }
  if (m.vlanIpCidr.isEmpty()) {
    if (error) *error = "vlan_ip_cidr 不能为空";
    return false;
  }
  return true;
}

QStringList ConfigTab::buildInitCommands() const {
  const auto m = initModel();
  const QString qosMap = "0:0 1:1 2:2 3:3 4:4 5:5 6:6 7:7";

  QStringList cmds;
  cmds << QString("ip addr replace %1 dev %2").arg(m.phyIpCidr, m.phyIface);
  cmds << QString("ip link set %1 up").arg(m.phyIface);
  cmds << QString("ip link show %1 >/dev/null 2>&1 || ip link add link %2 name %1 type vlan id %3 egress-qos-map %4")
              .arg(m.vlanIface, m.phyIface)
              .arg(m.vlanId)
              .arg(qosMap);
  cmds << QString("ip link set %1 up").arg(m.vlanIface);
  cmds << QString("ip addr replace %1 dev %2").arg(m.vlanIpCidr, m.vlanIface);
  return cmds;
}

bool ConfigTab::writeScript(const QString &path, const QStringList &commands,
                            QString *error) const {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    if (error) *error = QString("无法写入脚本：%1").arg(path);
    return false;
  }
  QTextStream out(&f);
  out << "#!/usr/bin/env bash\n";
  out << "set -euo pipefail\n\n";
  for (const auto &c : commands) out << c << "\n";
  f.close();
  f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                   QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther |
                   QFile::ExeOther);
  return true;
}

bool ConfigTab::runScript(const QString &path, QString *stdoutText,
                          QString *stderrText, int *exitCode) const {
  QProcess p;
  p.setProgram("/usr/bin/env");
  p.setArguments(QStringList() << "bash" << path);
  p.setProcessChannelMode(QProcess::SeparateChannels);
  p.start();
  if (!p.waitForStarted(3000)) {
    if (stderrText) *stderrText = "无法启动 bash 执行脚本";
    if (exitCode) *exitCode = -1;
    return false;
  }
  p.waitForFinished(-1);
  if (stdoutText) *stdoutText = QString::fromLocal8Bit(p.readAllStandardOutput());
  if (stderrText) *stderrText = QString::fromLocal8Bit(p.readAllStandardError());
  if (exitCode) *exitCode = p.exitCode();
  return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

static const char *kDefaultGptpConf =
    "[global]\n"
    "time_stamping           hardware\n"
    "twoStepFlag             1\n"
    "network_transport       L2\n"
    "domainNumber            0\n"
    "priority1               10\n"
    "priority2               128\n"
    "utc_offset 37\n"
    "logSyncInterval         0\n"
    "logAnnounceInterval     0\n"
    "logMinDelayReqInterval  0\n";

QString ConfigTab::gptpConfigPath() const {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      QDir(appDir).filePath("../configs/gptp.conf"),
      QDir(appDir).filePath("configs/gptp.conf"),
      QDir::current().filePath("configs/gptp.conf"),
      QDir::current().filePath("../configs/gptp.conf"),
      QDir::current().filePath("gptp.conf"),
  };
  for (const auto &path : candidates) {
    if (QFileInfo::exists(path)) return QFileInfo(path).absoluteFilePath();
  }
  return QFileInfo(candidates.front()).absoluteFilePath();
}

bool ConfigTab::ensureGptpConfig(QString *error) const {
  const QString path = gptpConfigPath();
  QFile f(path);
  if (f.exists()) return true;

  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    if (error) *error = QString("无法创建 gptp.conf：%1").arg(path);
    return false;
  }
  f.write(kDefaultGptpConf);
  f.close();
  return true;
}

bool ConfigTab::updateGptpPriority1(int priority, QString *error) const {
  const QString path = gptpConfigPath();
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (error) *error = QString("无法读取 gptp.conf：%1").arg(path);
    return false;
  }
  QStringList lines = QString::fromLocal8Bit(f.readAll()).split('\n');
  f.close();

  bool replaced = false;
  for (auto &line : lines) {
    const QString trimmed = line.trimmed();
    if (trimmed.startsWith("priority1")) {
      line = QString("priority1               %1").arg(priority);
      replaced = true;
      break;
    }
  }
  if (!replaced) {
    lines.prepend(QString("priority1               %1").arg(priority));
  }

  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    if (error) *error = QString("无法写入 gptp.conf：%1").arg(path);
    return false;
  }
  QTextStream out(&f);
  for (int i = 0; i < lines.size(); ++i) {
    out << lines.at(i);
    if (i != lines.size() - 1) out << "\n";
  }
  f.close();
  return true;
}

bool ConfigTab::startDetached(const QString &command, QString *error) const {
  const bool ok =
      QProcess::startDetached("bash", QStringList() << "-c" << command);
  if (!ok && error) {
    *error = QString("无法启动命令：\n%1").arg(command);
  }
  return ok;
}

static bool taiNowNs(qint64 *ns, QString *error) {
  struct timespec ts;
  if (clock_gettime(CLOCK_TAI, &ts) != 0) {
    if (error)
      *error = QString("获取 CLOCK_TAI 失败：%1").arg(
          QString::fromLocal8Bit(strerror(errno)));
    return false;
  }
  *ns = static_cast<qint64>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
  return true;
}

bool ConfigTab::isFollowerRole() const {
  return (btnRoleBackup_ && btnRoleBackup_->isChecked()) ||
         (btnRoleSlave_ && btnRoleSlave_->isChecked());
}

bool ConfigTab::isPtp4lRunning() const {
  QProcess p;
  p.start("pgrep", QStringList() << "-x" << "ts_masterd");
  if (!p.waitForFinished(1500)) return false;
  return p.exitCode() == 0;
}

void ConfigTab::updateStartButtonState() {
  const bool disable = timeSyncActive_ || isPtp4lRunning();
  if (btnStartTimeSync_) btnStartTimeSync_->setEnabled(!disable);
}

QString ConfigTab::currentPhcDevice() const {
  if (!phcDevicePath_.isEmpty()) return phcDevicePath_;
  return guessDefaultPhcDevice();
}

void ConfigTab::refreshPhcTimeInternal(bool showError) {
  const QString device = currentPhcDevice();
  if (device.isEmpty()) return;

  qint64 ns = 0;
  QString error;
  if (!readPhcTimeNs(device, &ns, &error)) {
    if (phcTimeLabel_) phcTimeLabel_->setText("PHC 时间：--");
    if (showError) QMessageBox::warning(this, "读取失败", error);
    return;
  }

  if (phcTimeLabel_) phcTimeLabel_->setText(QString("PHC 时间：%1").arg(ns));
}

void ConfigTab::refreshPhcTime() { refreshPhcTimeInternal(true); }

void ConfigTab::applyPhcTime() {
  const QString device = currentPhcDevice();
  if (device.isEmpty()) return;

  if (!phcAdjustSecondsEdit_) return;
  const QString text = phcAdjustSecondsEdit_->text().trimmed();
  if (text.isEmpty()) {
    QMessageBox::warning(this, "设置失败", "请输入调整秒数。");
    return;
  }
  bool ok = false;
  const qint64 deltaSeconds = text.toLongLong(&ok);
  if (!ok) {
    QMessageBox::warning(this, "设置失败", "请输入合法的整数秒（可为负数）。");
    return;
  }

  qint64 currentNs = 0;
  QString readError;
  if (!readPhcTimeNs(device, &currentNs, &readError)) {
    QMessageBox::warning(this, "读取失败", readError);
    return;
  }

  QString error;
  const qint64 targetNs = currentNs + deltaSeconds * 1000000000LL;
  if (!writePhcTimeNs(device, targetNs, &error)) {
    QMessageBox::warning(this, "设置失败", error);
    return;
  }

  refreshPhcTimeInternal(false);
  QMessageBox::information(this, "已设置",
                           QString("已将 %1 调整 %2 秒，新的 PHC 时间：%3")
                               .arg(device)
                               .arg(deltaSeconds)
                               .arg(QString::number(targetNs)));
}

void ConfigTab::handleRoleToggled(bool checked) {
  if (!checked) return;
  QString newRole = "master";
  if (btnRoleBackup_ && btnRoleBackup_->isChecked()) newRole = "backup";
  if (btnRoleSlave_ && btnRoleSlave_->isChecked()) newRole = "slave";
  const bool changed = newRole != currentRole_;
  currentRole_ = newRole;
  updateSyncAccuracyVisibility();

  if (suppressRoleRestart_) return;
  Q_UNUSED(changed);
}

void ConfigTab::updateSyncAccuracyVisibility() {
  const bool visible =
      lastPtpRoleFromLog_ == "slave" && !lastOffsetFromLog_.isEmpty();
  if (syncAccuracyLabel_) {
    syncAccuracyLabel_->setVisible(visible);
    if (!visible) {
      syncAccuracyLabel_->setText("同步精度：--");
    } else {
      syncAccuracyLabel_->setText(QString("同步精度：%1 ns").arg(lastOffsetFromLog_));
    }
  }
  if (systemSyncAccuracyLabel_) {
    const bool sysVisible = !lastSystemOffsetFromLog_.isEmpty();
    systemSyncAccuracyLabel_->setVisible(sysVisible);
    if (!sysVisible) {
      systemSyncAccuracyLabel_->setText("系统同步精度：--");
    } else {
      systemSyncAccuracyLabel_->setText(
          QString("系统同步精度：%1 ns").arg(lastSystemOffsetFromLog_));
    }
  }
  updateStartButtonState();
}

QWidget *ConfigTab::buildCbsPage(QWidget *parent) {
  auto *page = new QWidget(parent);
  auto *layout = new QVBoxLayout(page);

  cbsTable_ = new QTableWidget(8, 1, page);
  cbsTable_->setHorizontalHeaderLabels(QStringList() << "空闲斜率（单位为kbps）");
  QStringList verticalHeaders;
  for (int i = 0; i < 8; ++i) {
    verticalHeaders << QString("队列 %1").arg(i + 1);
  }
  cbsTable_->setVerticalHeaderLabels(verticalHeaders);
  cbsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  cbsTable_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  for (int row = 0; row < 8; ++row) {
    auto *spin = new QSpinBox(cbsTable_);
    spin->setRange(-2000000000, 2000000000);
    spin->setValue(0);
    cbsTable_->setCellWidget(row, 0, spin);
  }

  // 保证 8 个队列完整可见
  const auto setMinHeight = [](QTableWidget *table, int rows) {
    const int rowHeight = table->verticalHeader()->defaultSectionSize();
    const int headerHeight = table->horizontalHeader()->height();
    table->setMinimumHeight(headerHeight + rowHeight * rows + 8);
  };
  setMinHeight(cbsTable_, 8);

  auto *resetBtn = new QPushButton("全部重置为 0", page);
  layout->addWidget(new QLabel("CBS 参数", page));
  layout->addWidget(cbsTable_);
  layout->addWidget(resetBtn);
  layout->addStretch(1);

  connect(resetBtn, &QPushButton::clicked, this, &ConfigTab::resetCbs);

  return page;
}

QWidget *ConfigTab::buildTasPage(QWidget *parent) {
  auto *page = new QWidget(parent);
  auto *layout = new QVBoxLayout(page);

  tasDelay_ = new QSpinBox(page);
  tasDelay_->setRange(0, 3600000);
  tasDelay_->setSuffix(" ms");
  tasDelay_->setToolTip("以当前时间为基准，增加的延迟时间（毫秒）用来生成 GCL 生效时间 base-time");

  auto *formLayout = new QFormLayout();
  formLayout->addRow("GCL 生效时间", tasDelay_);

  tasTable_ = new QTableWidget(0, 2, page);
  tasTable_->setHorizontalHeaderLabels(
      QStringList() << "开门队列掩码 (0x..)" << "开门时间 (ns)");
  tasTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  tasTable_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tasTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tasTable_->setSelectionMode(QAbstractItemView::SingleSelection);

  auto *tasButtonsLayout = new QHBoxLayout();
  auto *addBtn = new QPushButton("添加行", page);
  auto *removeBtn = new QPushButton("删除选中行", page);
  gclBaseTimeLabel_ = new QLabel("GCL 生效时间：--", page);
  gclBaseTimeLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  tasButtonsLayout->addWidget(addBtn);
  tasButtonsLayout->addWidget(removeBtn);
  tasButtonsLayout->addWidget(gclBaseTimeLabel_);
  tasButtonsLayout->addStretch(1);

  layout->addLayout(formLayout);
  layout->addWidget(new QLabel("GCL 门控配置", page));
  layout->addWidget(tasTable_);
  layout->addLayout(tasButtonsLayout);
  layout->addStretch(1);

  // 与 CBS 匹配的最小高度，避免行数被裁切
  const auto setMinHeight = [](QTableWidget *table, int rows) {
    const int rowHeight = table->verticalHeader()->defaultSectionSize();
    const int headerHeight = table->horizontalHeader()->height();
    table->setMinimumHeight(headerHeight + rowHeight * rows + 8);
  };
  setMinHeight(tasTable_, 8);

  connect(addBtn, &QPushButton::clicked, this, &ConfigTab::addTasRow);
  connect(removeBtn, &QPushButton::clicked, this, &ConfigTab::removeSelectedTasRow);

  addTasRow();
  return page;
}

void ConfigTab::onShapingModeChanged() {
  if (!shapeStack_) return;
  shapeStack_->setCurrentIndex(cbsRadio_ && cbsRadio_->isChecked() ? 0 : 1);
}

void ConfigTab::addTasRow() {
  if (!tasTable_) return;

  const int row = tasTable_->rowCount();
  tasTable_->insertRow(row);

  auto *maskEdit = new QLineEdit(tasTable_);
  maskEdit->setPlaceholderText("例如：0xFF");
  maskEdit->setText("0");
  maskEdit->setValidator(new QRegularExpressionValidator(
      QRegularExpression("0x?[0-9A-Fa-f]{1,8}"), maskEdit));

  auto *timeEdit = new QLineEdit(tasTable_);
  timeEdit->setPlaceholderText("例如：2500000");
  timeEdit->setText("0");
  timeEdit->setValidator(new QIntValidator(0, 2000000000, timeEdit));

  tasTable_->setCellWidget(row, 0, maskEdit);
  tasTable_->setCellWidget(row, 1, timeEdit);
}

void ConfigTab::removeSelectedTasRow() {
  if (!tasTable_ || !tasTable_->selectionModel()) return;
  const auto rows = tasTable_->selectionModel()->selectedRows();
  for (int i = rows.size() - 1; i >= 0; --i) {
    tasTable_->removeRow(rows.at(i).row());
  }
}

void ConfigTab::resetCbs() {
  if (!cbsTable_) return;
  for (int row = 0; row < cbsTable_->rowCount(); ++row) {
    if (auto *spin = qobject_cast<QSpinBox *>(cbsTable_->cellWidget(row, 0))) {
      spin->setValue(0);
    }
  }
}

QStringList ConfigTab::buildCbsCommands(const QString &iface, QString *error) const {
  Q_UNUSED(error);
  QStringList commands;
  const QString mqprioCmd =
      QString("sudo tc qdisc replace dev %1 root handle 100 mqprio "
              "num_tc 8 "
              "map 7 6 5 4 3 2 1 0 "
              "queues 1@0 1@1 1@2 1@3 1@4 1@5 1@6 1@7 "
              "hw 0")
          .arg(iface);
  commands << mqprioCmd;

  // Derive tc cbs parameters so idleslope + sendslope = 1,000,000.
  const qint64 portRateBps = 1000000LL;
  const int lineRateFrameBytes = 1542;  // 1500B payload + VLAN + preamble + IFG
  const auto creditFromSlope = [&](qint64 slope) -> qint64 {
    return (slope * lineRateFrameBytes) / portRateBps;
  };

  for (int row = 0; row < 8; ++row) {
    auto *idleslopeSpin = qobject_cast<QSpinBox *>(cbsTable_->cellWidget(row, 0));
    const qint64 idleslope = idleslopeSpin ? idleslopeSpin->value() : 0;
    const qint64 sendslope = idleslope - portRateBps;
    const qint64 locredit = creditFromSlope(sendslope);
    const qint64 hicredit = creditFromSlope(idleslope);

    const int handleBase = 110 + row * 10;
    const QString cmd =
        QString("sudo tc qdisc replace dev %1 parent 100:%2 handle %3: cbs "
                "locredit %4 hicredit %5 sendslope %6 idleslope %7")
            .arg(iface)
            .arg(row + 1)
            .arg(handleBase)
            .arg(locredit)
            .arg(hicredit)
            .arg(sendslope)
            .arg(idleslope);
    commands << cmd;
  }
  return commands;
}

QStringList ConfigTab::buildTasCommands(const QString &iface, QString *error,
                                        qint64 *baseTimeNs) const {
  if (baseTimeNs) *baseTimeNs = 0;
  QStringList commands;
  if (!tasTable_ || tasTable_->rowCount() == 0) {
    if (error) *error = "请至少添加一条 sched-entry。";
    return commands;
  }

  qint64 taiNs = 0;
  if (!taiNowNs(&taiNs, error)) {
    return commands;
  }

  const qint64 delayNs =
      tasDelay_ ? static_cast<qint64>(tasDelay_->value()) * 1000000LL : 0;
  const qint64 baseTime = taiNs + delayNs;
  if (baseTimeNs) *baseTimeNs = baseTime;

  QString cmd =
      QString("sudo tc qdisc replace dev %1 root handle 100 taprio "
              "num_tc 8 "
              "map 7 6 5 4 3 2 1 0 "
              "queues 1@0 1@1 1@2 1@3 1@4 1@5 1@6 1@7 "
              "base-time %2 ")
          .arg(iface)
          .arg(baseTime);

  for (int row = 0; row < tasTable_->rowCount(); ++row) {
    auto *maskEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 0));
    auto *timeEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 1));
    const QString maskText = maskEdit ? maskEdit->text().trimmed() : QString();
    const QString timeText = timeEdit ? timeEdit->text().trimmed() : QString();

    bool maskOk = false;
    const int mask = maskText.isEmpty() ? 0 : maskText.toInt(&maskOk, 0);
    if (!maskOk || mask < 0 || mask > 0xFF) {
      if (error) *error =
                    QString("第 %1 行队列掩码格式错误，请输入 0x00~0xFF。")
                        .arg(row + 1);
      return {};
    }

    bool timeOk = false;
    const qint64 interval = timeText.toLongLong(&timeOk);
    if (!timeOk || interval <= 0) {
      if (error) *error =
                    QString("第 %1 行开门时间需为正整数（ns）。").arg(row + 1);
      return {};
    }

    const QString maskStr =
        QString("0x%1").arg(mask, 2, 16, QLatin1Char('0')).toUpper();
    cmd += QString(" sched-entry S %1 %2").arg(maskStr).arg(interval);
  }

  cmd += " clockid CLOCK_TAI";
  commands << cmd;
  return commands;
}

bool ConfigTab::runCommand(const QString &command, QString *error) const {
  QProcess process;
  process.start("bash", QStringList() << "-c" << command);
  if (!process.waitForFinished(-1)) {
    if (error) *error = QString("命令超时：\n%1").arg(command);
    return false;
  }

  const QString stdoutText =
      QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
  const QString stderrText =
      QString::fromLocal8Bit(process.readAllStandardError()).trimmed();

  if (process.exitCode() != 0) {
    QString detail = QString("命令执行失败（退出码 %1）：\n%2")
                         .arg(process.exitCode())
                         .arg(command);
    if (!stderrText.isEmpty()) {
      detail += "\n\nstderr:\n" + stderrText;
    } else if (!stdoutText.isEmpty()) {
      detail += "\n\nstdout:\n" + stdoutText;
    }
    if (error) *error = detail;
    return false;
  }
  return true;
}

bool ConfigTab::runCommands(const QStringList &commands, QString *error) const {
  for (const auto &cmd : commands) {
    if (!runCommand(cmd, error)) {
      if (error && error->isEmpty())
        *error = QString("命令执行失败：\n%1").arg(cmd);
      return false;
    }
  }
  return true;
}

bool ConfigTab::clearExistingQdiscIfNeeded(const QString &iface,
                                           QString *error) const {
  const QString showCmd = QString("sudo tc qdisc show dev %1").arg(iface);
  QProcess showProcess;
  showProcess.start("bash", QStringList() << "-c" << showCmd);
  if (!showProcess.waitForFinished(-1)) {
    if (error) *error = QString("命令超时：\n%1").arg(showCmd);
    return false;
  }

  const QString stdoutText =
      QString::fromLocal8Bit(showProcess.readAllStandardOutput()).trimmed();
  const QString stderrText =
      QString::fromLocal8Bit(showProcess.readAllStandardError()).trimmed();

  if (showProcess.exitCode() != 0) {
    QString detail = QString("命令执行失败（退出码 %1）：\n%2")
                         .arg(showProcess.exitCode())
                         .arg(showCmd);
    if (!stderrText.isEmpty()) {
      detail += "\n\nstderr:\n" + stderrText;
    } else if (!stdoutText.isEmpty()) {
      detail += "\n\nstdout:\n" + stdoutText;
    }
    if (error) *error = detail;
    return false;
  }

  const QString combined = (stdoutText + "\n" + stderrText).toLower();
  if (!combined.contains("taprio") && !combined.contains("mqprio")) {
    return true;
  }

  return runCommand(QString("sudo tc qdisc del dev %1 root").arg(iface), error);
}

bool ConfigTab::startTerminalScript(const QString &script,
                                    QString *error) const {
  struct Candidate {
    QString program;
    QStringList args;
  };

  QList<Candidate> candidates;
  candidates << Candidate{"x-terminal-emulator",
                          QStringList() << "-e"
                                        << "bash"
                                        << "-lc" << script}
             << Candidate{"gnome-terminal",
                          QStringList() << "--" << "bash"
                                        << "-lc" << script}
             << Candidate{"konsole",
                          QStringList() << "-e"
                                        << "bash"
                                        << "-lc" << script}
             << Candidate{"xfce4-terminal",
                          QStringList() << "-e"
                                        << "bash"
                                        << "-lc" << script}
             << Candidate{"xterm",
                          QStringList() << "-hold"
                                        << "-e"
                                        << "bash"
                                        << "-lc" << script};

  for (const auto &c : candidates) {
    if (QProcess::startDetached(c.program, c.args)) return true;
  }

  if (QProcess::startDetached("bash", QStringList() << "-lc" << script)) {
    return true;
  }

  if (error)
    *error = "无法启动终端窗口执行命令，请确认已安装终端程序（xterm/gnome-terminal 等）。";
  return false;
}

void ConfigTab::applyInitConfig() {
  QString error;
  if (!validateInit(&error)) {
    QMessageBox::warning(this, "配置失败", error);
    return;
  }
  const auto m = initModel();
  if (!isInterfaceUp(m.phyIface, &error)) {
    QMessageBox::warning(this, "配置失败", error);
    return;
  }
  if (geteuid() != 0) {
    QMessageBox::warning(this, "配置失败", "请用 sudo 启动本程序后再执行配置。");
    return;
  }

  btnInitApply_->setEnabled(false);

  QDir scriptDir(QCoreApplication::applicationDirPath());
  if (scriptDir.dirName() == "bin") {
    scriptDir.cdUp();  // keep script at project root (TSN_SoftWare)
  }
  const QString scriptPath = scriptDir.filePath("generated_init.sh");
  if (!writeScript(scriptPath, buildInitCommands(), &error)) {
    btnInitApply_->setEnabled(true);
    QMessageBox::critical(this, "配置失败", error);
    return;
  }

  QString outText;
  QString errText;
  int exitCode = 0;
  const bool ok = runScript(scriptPath, &outText, &errText, &exitCode);
  btnInitApply_->setEnabled(true);

  if (ok) {
    if (qApp) qApp->setProperty("init_config_done", true);
    QMessageBox::information(this, "配置成功", "初始化配置成功。");
    return;
  }

  QString detail = errText.trimmed();
  if (detail.isEmpty()) detail = outText.trimmed();
  if (detail.size() > 2000) detail = detail.left(2000) + "\n...(truncated)";
  QMessageBox::critical(
      this, "配置失败",
      QString("初始化配置失败（exitCode=%1）。\n\n%2")
          .arg(exitCode)
          .arg(detail.isEmpty() ? "无输出。" : detail));
}

void ConfigTab::startTimeSync() {
  const auto restoreButton = [this]() {
    updateStartButtonState();
  };
  if (btnStartTimeSync_) btnStartTimeSync_->setEnabled(false);

  if (!qApp || !qApp->property("init_config_done").toBool()) {
    restoreButton();
    QMessageBox::warning(this, "配置失败", "请先初始化配置");
    return;
  }

  if (isPtp4lRunning()) {
    restoreButton();
    QMessageBox::information(this, "提示", "已停止时间同步，并成功切换角色。");
    return;
  }

  const QString iface =
      phyIface_ ? phyIface_->currentData().toString().trimmed() : QString();
  if (iface.isEmpty()) {
    restoreButton();
    QMessageBox::warning(this, "配置失败", "请在初始化配置中选择网卡 phy_iface。");
    return;
  }

  QString ifaceError;
  if (!isInterfaceUp(iface, &ifaceError)) {
    restoreButton();
    QMessageBox::warning(this, "配置失败", ifaceError);
    return;
  }

  const bool isSlave = btnRoleSlave_ && btnRoleSlave_->isChecked();

  int priority = 0;
  if (btnRoleMaster_ && btnRoleMaster_->isChecked()) {
    priority = 1;
  } else if (btnRoleBackup_ && btnRoleBackup_->isChecked()) {
    priority = 10;
  } else if (btnRoleSlave_ && btnRoleSlave_->isChecked()) {
    priority = 100;
  } else {
    restoreButton();
    QMessageBox::warning(this, "配置失败", "请选择节点角色：主 / 备用 / 从。");
    return;
  }

  QString error;
  if (!ensureGptpConfig(&error) || !updateGptpPriority1(priority, &error)) {
    restoreButton();
    QMessageBox::warning(this, "配置失败", error);
    return;
  }

  lastPtpRoleFromLog_ = "unknown";
  lastOffsetFromLog_.clear();
  lastSystemOffsetFromLog_.clear();
  ptpBuffer_.clear();
  phcBuffer_.clear();
  updateSyncAccuracyVisibility();
  if (!runCommands(
          {"sudo -n systemctl stop systemd-timesyncd || true",
           "sudo -n systemctl disable systemd-timesyncd || true",
           "sudo -n systemctl stop chronyd || true",
           "sudo -n systemctl disable chronyd || true",
           "sudo -n systemctl stop ntp || true",
           "sudo -n systemctl disable ntp || true"},
          &error)) {
    restoreButton();
    QMessageBox::warning(this, "配置失败", error);
    return;
  }

  if (ptpProcess_) {
    ptpProcess_->kill();
    ptpProcess_->deleteLater();
    ptpProcess_ = nullptr;
  }
  if (phcProcess_) {
    phcProcess_->kill();
    phcProcess_->deleteLater();
    phcProcess_ = nullptr;
  }

  ptpBuffer_.clear();
  phcBuffer_.clear();
  lastSystemOffsetFromLog_.clear();
  updateSyncAccuracyVisibility();

  const bool isRoot = (geteuid() == 0);
  const QString gptpPath = gptpConfigPath();
  const QString tsMasterPath = findTimesyncBinary("ts_masterd");
  ptpProcess_ = new QProcess(this);
  ptpProcess_->setProgram(isRoot ? tsMasterPath : "sudo");
  QStringList ptpArgs;
  if (!isRoot) ptpArgs << "-n";
  ptpArgs << tsMasterPath << "-f" << gptpPath << "-i" << iface << "-m";
  ptpProcess_->setArguments(ptpArgs);
  ptpProcess_->setProcessChannelMode(QProcess::MergedChannels);
  connect(ptpProcess_, &QProcess::readyRead, this, &ConfigTab::handlePtpOutput);
  connect(ptpProcess_,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ConfigTab::handlePtpFinished);

  if (btnStartTimeSync_) btnStartTimeSync_->setEnabled(false);
  systemSyncScheduled_ = true;
  timeSyncActive_ = true;

  ptpProcess_->start();
  if (!ptpProcess_->waitForStarted(3000)) {
    systemSyncScheduled_ = false;
    timeSyncActive_ = false;
    restoreButton();
    if (syncAccuracyLabel_) syncAccuracyLabel_->setText("同步精度：--");
    QMessageBox::warning(
        this, "配置失败",
        QString("无法启动 ts_masterd，请确认 sudo 权限和配置文件：%1").arg(gptpPath));
    ptpProcess_->deleteLater();
    ptpProcess_ = nullptr;
    return;
  }

  QTimer::singleShot(4000, this, [this, iface, isSlave, restoreButton]() {
    if (!systemSyncScheduled_) {
      restoreButton();
      return;
    }

    const QString pmcPath = findTimesyncBinary("ts_ctl");
    const QString pmcCommand =
        "SET GRANDMASTER_SETTINGS_NP clockClass 248 clockAccuracy 0xfe "
        "offsetScaledLogVariance 0xffff currentUtcOffset 37 leap61 0 "
        "leap59 0 currentUtcOffsetValid 1 ptpTimescale 1 timeTraceable 1 "
        "frequencyTraceable 0 timeSource 0xa0";
    const QString pmcCmd = (geteuid() == 0)
                               ? QString("\"%1\" -u -s /var/run/ts_masterd -b 0 \"%2\"")
                                     .arg(pmcPath, pmcCommand)
                               : QString("sudo \"%1\" -u -s /var/run/ts_masterd -b 0 \"%2\"")
                                     .arg(pmcPath, pmcCommand);
    QString pmcError;
    runCommand(pmcCmd, &pmcError);  // 保留输出以便定位 sudo/pmc 失败

    QString error;
    if (!startSystemSyncInternal(iface, isSlave, &error)) {
      systemSyncScheduled_ = false;
      timeSyncActive_ = false;
      QMessageBox::warning(this, "配置失败", error);
      restoreButton();
      return;
    }
    systemSyncScheduled_ = false;
    QMessageBox::information(this, "同步成功", "时间同步和系统时间同步已启动。");
  });
}

void ConfigTab::handlePtpOutput() {
  if (!ptpProcess_) return;
  const QString data = ptpBuffer_ + QString::fromLocal8Bit(ptpProcess_->readAll());
  QStringList lines = data.split('\n');
  ptpBuffer_.clear();
  if (!data.endsWith('\n') && !lines.isEmpty()) {
    ptpBuffer_ = lines.takeLast();
  } else if (!lines.isEmpty()) {
    lines.removeLast();
  }

  static const QRegularExpression kOffsetRegex("\\bmaster\\s+offset\\s+(-?\\d+)");
  for (const auto &line : lines) {
    const QString trimmed = line.trimmed();
    if (trimmed.contains("to MASTER")) {
      lastPtpRoleFromLog_ = "master";
      lastOffsetFromLog_.clear();
    } else if (trimmed.contains("to SLAVE")) {
      lastPtpRoleFromLog_ = "slave";
    }
  }

  for (const auto &line : lines) {
    const auto match = kOffsetRegex.match(line);
    if (match.hasMatch()) {
      lastOffsetFromLog_ = match.captured(1);
    }
  }

  updateSyncAccuracyVisibility();
}

void ConfigTab::handlePhcOutput() {
  if (!phcProcess_) return;
  const QString data = phcBuffer_ + QString::fromLocal8Bit(phcProcess_->readAll());
  QStringList lines = data.split('\n');
  phcBuffer_.clear();
  if (!data.endsWith('\n') && !lines.isEmpty()) {
    phcBuffer_ = lines.takeLast();
  } else if (!lines.isEmpty()) {
    lines.removeLast();
  }

  static const QRegularExpression kPhcOffsetRegex("\\boffset\\s+(-?\\d+)\\b");
  for (const auto &line : lines) {
    const auto match = kPhcOffsetRegex.match(line);
    if (match.hasMatch()) {
      lastSystemOffsetFromLog_ = match.captured(1);
    }
  }

  updateSyncAccuracyVisibility();
}

void ConfigTab::handlePtpFinished(int exitCode, QProcess::ExitStatus status) {
  Q_UNUSED(exitCode);
  Q_UNUSED(status);
  systemSyncScheduled_ = false;
  timeSyncActive_ = false;
  if (btnStartTimeSync_) btnStartTimeSync_->setEnabled(true);
  lastPtpRoleFromLog_ = "unknown";
  lastOffsetFromLog_.clear();
  updateSyncAccuracyVisibility();
  updateStartButtonState();
  if (ptpProcess_) {
    ptpProcess_->deleteLater();
    ptpProcess_ = nullptr;
  }
}

bool ConfigTab::stopTimeSyncInternal(bool showMessage) {
  systemSyncScheduled_ = false;
  timeSyncActive_ = false;
  if (btnStartTimeSync_) btnStartTimeSync_->setEnabled(true);

  if (ptpProcess_) {
    ptpProcess_->kill();
    ptpProcess_->deleteLater();
    ptpProcess_ = nullptr;
  }
  if (phcProcess_) {
    phcProcess_->kill();
    phcProcess_->deleteLater();
    phcProcess_ = nullptr;
  }
  ptpBuffer_.clear();
  phcBuffer_.clear();
  lastPtpRoleFromLog_ = "unknown";
  lastOffsetFromLog_.clear();
  lastSystemOffsetFromLog_.clear();
  updateSyncAccuracyVisibility();
  if (syncAccuracyLabel_) syncAccuracyLabel_->setText("同步精度：--");
  if (systemSyncAccuracyLabel_) systemSyncAccuracyLabel_->setText("系统同步精度：--");

  QString error;
  if (!runCommands({"sudo -n pkill ts_masterd || true", "sudo -n pkill clk_bridge || true"}, &error)) {
    if (showMessage) QMessageBox::warning(this, "停止失败", error);
    return false;
  }
  if (showMessage) QMessageBox::information(this, "已停止", "时间同步已停止。");
  updateStartButtonState();
  return true;
}

void ConfigTab::stopTimeSync() { stopTimeSyncInternal(true); }

void ConfigTab::switchRole() {
  if (btnSwitchRole_) btnSwitchRole_->setEnabled(false);
  const bool wasActive = timeSyncActive_;
  stopTimeSyncInternal(false);
  startTimeSync();
  if (timeSyncActive_) {
    QMessageBox::information(this, "角色切换成功", "已成功切换时间同步角色。");
  } else if (!wasActive) {
    QMessageBox::information(this, "提示", "已启动时间同步。");
  }
  if (btnSwitchRole_) btnSwitchRole_->setEnabled(true);
}

bool ConfigTab::startSystemSyncInternal(const QString &iface, bool isSlave,
                                        QString *error) {
  if (phcProcess_) {
    phcProcess_->kill();
    phcProcess_->deleteLater();
    phcProcess_ = nullptr;
  }
  phcBuffer_.clear();
  lastSystemOffsetFromLog_.clear();
  updateSyncAccuracyVisibility();

  Q_UNUSED(isSlave);
  const bool isRoot = (geteuid() == 0);
  const QString clkBridgePath = findTimesyncBinary("clk_bridge");
  QStringList args;
  if (!isRoot) args << "-n";
  args << clkBridgePath << "-s" << iface << "-c"
       << "CLOCK_REALTIME"
       << "-m"
       << "-w"
       << "--step_threshold=1000";

  phcProcess_ = new QProcess(this);
  phcProcess_->setProgram(isRoot ? clkBridgePath : "sudo");
  phcProcess_->setArguments(args);
  phcProcess_->setProcessChannelMode(QProcess::MergedChannels);
  connect(phcProcess_, &QProcess::readyRead, this, &ConfigTab::handlePhcOutput);
  connect(phcProcess_,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this](int, QProcess::ExitStatus) {
            phcBuffer_.clear();
            lastSystemOffsetFromLog_.clear();
            updateSyncAccuracyVisibility();
            if (phcProcess_) {
              phcProcess_->deleteLater();
              phcProcess_ = nullptr;
            }
          });
  phcProcess_->start();
  if (!phcProcess_->waitForStarted(3000)) {
    if (error) *error = "无法启动 clk_bridge，请检查 sudo 权限。";
    phcProcess_->deleteLater();
    phcProcess_ = nullptr;
    return false;
  }
  return true;
}

QJsonObject ConfigTab::buildSchemeJson() const {
  QJsonObject obj;

  QJsonObject init;
  const auto m = initModel();
  init["phy_iface"] = m.phyIface;
  init["phy_ip_cidr"] = m.phyIpCidr;
  init["vlan_id"] = m.vlanId;
  init["vlan_iface"] = m.vlanIface;
  init["vlan_ip_cidr"] = m.vlanIpCidr;
  obj["init"] = init;

  QJsonObject timeSync;
  QString role = "master";
  if (btnRoleBackup_ && btnRoleBackup_->isChecked()) role = "backup";
  if (btnRoleSlave_ && btnRoleSlave_->isChecked()) role = "slave";
  timeSync["role"] = role;
  obj["time_sync"] = timeSync;

  QJsonObject shaping;
  shaping["mode"] = cbsRadio_ && cbsRadio_->isChecked() ? "cbs" : "tas";

  QJsonArray cbsArray;
  if (cbsTable_) {
    for (int row = 0; row < cbsTable_->rowCount(); ++row) {
      auto *spin = qobject_cast<QSpinBox *>(cbsTable_->cellWidget(row, 0));
      cbsArray.append(spin ? spin->value() : 0);
    }
  }
  shaping["cbs_idleslope"] = cbsArray;

  QJsonObject tas;
  tas["delay_ms"] = tasDelay_ ? tasDelay_->value() : 0;
  QJsonArray entries;
  if (tasTable_) {
    for (int row = 0; row < tasTable_->rowCount(); ++row) {
      auto *maskEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 0));
      auto *timeEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 1));
      QJsonObject entry;
      entry["mask"] = maskEdit ? maskEdit->text().trimmed() : "";
      entry["interval_ns"] = timeEdit ? timeEdit->text().trimmed() : "";
      entries.append(entry);
    }
  }
  tas["entries"] = entries;
  shaping["tas"] = tas;
  obj["shaping"] = shaping;

  return obj;
}

bool ConfigTab::applySchemeJson(const QJsonObject &obj, QString *error) {
  // init
  const auto init = obj.value("init").toObject();
  const QString phyIface = init.value("phy_iface").toString();
  const QString phyIp = init.value("phy_ip_cidr").toString();
  const int vlanId = init.value("vlan_id").toInt(100);
  const QString vlanIface = init.value("vlan_iface").toString();
  const QString vlanIp = init.value("vlan_ip_cidr").toString();

  if (!phyIface.isEmpty() && phyIface_) {
    int idx = phyIface_->findData(phyIface);
    if (idx < 0) {
      phyIface_->addItem(phyIface, phyIface);
      idx = phyIface_->findData(phyIface);
    }
    if (idx >= 0) phyIface_->setCurrentIndex(idx);
  }
  if (phyIpCidr_) phyIpCidr_->setText(phyIp);
  if (vlanId_) vlanId_->setValue(vlanId);
  if (vlanIface_) {
    vlanIfaceManual_ = true;
    vlanIface_->setText(vlanIface);
  }
  if (vlanIpCidr_) vlanIpCidr_->setText(vlanIp);

  // time sync
  const QString role = obj.value("time_sync").toObject().value("role").toString("backup");
  const bool prevSuppress = suppressRoleRestart_;
  suppressRoleRestart_ = true;
  if (btnRoleMaster_) btnRoleMaster_->setChecked(role == "master");
  if (btnRoleBackup_) btnRoleBackup_->setChecked(role == "backup");
  if (btnRoleSlave_) btnRoleSlave_->setChecked(role == "slave");
  currentRole_ = role;
  suppressRoleRestart_ = prevSuppress;

  // shaping
  const auto shaping = obj.value("shaping").toObject();
  const QString mode = shaping.value("mode").toString("cbs");
  if (mode == "tas") {
    if (tasRadio_) tasRadio_->setChecked(true);
    if (cbsRadio_) cbsRadio_->setChecked(false);
  } else {
    if (cbsRadio_) cbsRadio_->setChecked(true);
    if (tasRadio_) tasRadio_->setChecked(false);
  }
  onShapingModeChanged();

  const auto cbsArr = shaping.value("cbs_idleslope").toArray();
  if (cbsTable_) {
    for (int row = 0; row < cbsTable_->rowCount(); ++row) {
      auto *spin = qobject_cast<QSpinBox *>(cbsTable_->cellWidget(row, 0));
      if (!spin) continue;
      if (row < cbsArr.size()) {
        spin->setValue(cbsArr.at(row).toInt());
      }
    }
  }

  const auto tasObj = shaping.value("tas").toObject();
  if (tasDelay_) {
    int delay = tasObj.value("delay_ms").toInt(0);
    delay = qBound(tasDelay_->minimum(), delay, tasDelay_->maximum());
    tasDelay_->setValue(delay);
  }

  if (tasTable_) {
    tasTable_->setRowCount(0);
    const auto entries = tasObj.value("entries").toArray();
    for (const auto &e : entries) {
      const auto entryObj = e.toObject();
      addTasRow();
      const int row = tasTable_->rowCount() - 1;
      auto *maskEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 0));
      auto *timeEdit = qobject_cast<QLineEdit *>(tasTable_->cellWidget(row, 1));
      if (maskEdit) maskEdit->setText(entryObj.value("mask").toString("0"));
      if (timeEdit) timeEdit->setText(entryObj.value("interval_ns").toString("0"));
    }
    if (tasTable_->rowCount() == 0) addTasRow();
  }

  updateDerived();
  updateSyncAccuracyVisibility();
  Q_UNUSED(error);
  return true;
}

void ConfigTab::saveScheme() {
  const QString path =
      QFileDialog::getSaveFileName(this, "保存方案", QString(),
                                   "TSN 方案 (*.json);;所有文件 (*)");
  if (path.isEmpty()) return;

  const QJsonObject obj = buildSchemeJson();
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QMessageBox::warning(this, "保存失败",
                         QString("无法写入文件：%1").arg(path));
    return;
  }
  f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
  f.close();
  QMessageBox::information(this, "已保存", "方案已保存。");
}

void ConfigTab::loadScheme() {
  const QString path =
      QFileDialog::getOpenFileName(this, "导入方案", QString(),
                                   "TSN 方案 (*.json);;所有文件 (*)");
  if (path.isEmpty()) return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "导入失败",
                         QString("无法读取文件：%1").arg(path));
    return;
  }
  const QByteArray data = f.readAll();
  f.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    QMessageBox::warning(this, "导入失败",
                         QString("JSON 解析失败：%1").arg(parseError.errorString()));
    return;
  }

  QString error;
  if (!applySchemeJson(doc.object(), &error)) {
    QMessageBox::warning(this, "导入失败", error.isEmpty() ? "未知错误" : error);
    return;
  }
  QMessageBox::information(this, "导入成功", "方案已导入。");
  updateStartButtonState();
}
void ConfigTab::applyShapingConfig() {
  const QString iface = phyIface_ ? phyIface_->currentData().toString().trimmed()
                                  : QString();
  if (iface.isEmpty()) {
    QMessageBox::warning(this, "配置失败", "请在初始化配置中选择网卡 phy_iface。");
    return;
  }
  if (!qApp || !qApp->property("init_config_done").toBool()) {
    QMessageBox::warning(this, "配置失败", "请先初始化配置");
    return;
  }

  QString ifaceError;
  if (!isInterfaceUp(iface, &ifaceError)) {
    QMessageBox::warning(this, "配置失败", ifaceError);
    return;
  }

  QString error;
  const bool useCbs = cbsRadio_ && cbsRadio_->isChecked();
  const bool useTas = !useCbs;
  qint64 baseTimeNs = 0;
  QStringList commands =
      useCbs ? buildCbsCommands(iface, &error)
             : buildTasCommands(iface, &error, &baseTimeNs);

  if (!error.isEmpty()) {
    QMessageBox::warning(this, "配置失败", error);
    return;
  }
  if (commands.isEmpty()) {
    QMessageBox::warning(this, "配置失败", "未生成任何命令，请检查输入。");
    return;
  }

  btnShapeApply_->setEnabled(false);
  if (!clearExistingQdiscIfNeeded(iface, &error)) {
    btnShapeApply_->setEnabled(true);
    QMessageBox::warning(this, "配置失败", error);
    return;
  }

  if (!runCommands(commands, &error)) {
    btnShapeApply_->setEnabled(true);
    QMessageBox::warning(this, "执行失败", error);
    return;
  }

  btnShapeApply_->setEnabled(true);
  if (useTas && gclBaseTimeLabel_) {
    if (baseTimeNs > 0) {
      const qint64 baseMs = baseTimeNs / 1000000LL;
      const QDateTime approxLocal =
          QDateTime::fromMSecsSinceEpoch(baseMs).toLocalTime();
      gclBaseTimeLabel_->setText(
          QString("GCL 生效时间：%1 ns（约 %2）")
              .arg(baseTimeNs)
              .arg(approxLocal.toString("yyyy-MM-dd HH:mm:ss.zzz")));
    } else {
      gclBaseTimeLabel_->setText("GCL 生效时间：--");
    }
  } else if (gclBaseTimeLabel_) {
    gclBaseTimeLabel_->setText("GCL 生效时间：--");
  }
  QMessageBox::information(this, "配置成功", "整形配置执行完成。");
}
