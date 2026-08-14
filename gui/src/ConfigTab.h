#pragma once

#include <QWidget>
#include <QProcess>

class QComboBox;
class QLineEdit;
class QRadioButton;
class QStackedWidget;
class QSpinBox;
class QTableWidget;
class QPushButton;
class QLabel;
class QJsonObject;
class QTimer;

class ConfigTab final : public QWidget {
  Q_OBJECT

 public:
  explicit ConfigTab(QWidget *parent = nullptr);

 private slots:
 void reloadInterfaces();
 void updateDerived();
 void applyInitConfig();
  void startTimeSync();
  void stopTimeSync();
  void applyShapingConfig();
  void onShapingModeChanged();
  void addTasRow();
  void removeSelectedTasRow();
  void resetCbs();
  void updateSyncAccuracyVisibility();
  void handlePtpOutput();
  void handlePhcOutput();
  void handlePtpFinished(int exitCode, QProcess::ExitStatus status);
  bool isFollowerRole() const;
 void saveScheme();
 void loadScheme();
  void handleRoleToggled(bool checked);
  void switchRole();
  void updateStartButtonState();
  void refreshPhcTime();
  void applyPhcTime();

 private:
  struct InitModel final {
    QString phyIface;
    QString phyIpCidr;
    int vlanId = 100;
    QString vlanIface;
    QString vlanIpCidr;
  };

  InitModel initModel() const;
  bool validateInit(QString *error) const;
  QStringList buildInitCommands() const;
  bool writeScript(const QString &path, const QStringList &commands,
                   QString *error) const;
  bool runScript(const QString &path, QString *stdoutText, QString *stderrText,
                 int *exitCode) const;
  bool ensureGptpConfig(QString *error) const;
  bool updateGptpPriority1(int priority, QString *error) const;
  QString gptpConfigPath() const;
  bool startDetached(const QString &command, QString *error) const;
  QWidget *buildCbsPage(QWidget *parent);
  QWidget *buildTasPage(QWidget *parent);
  QStringList buildCbsCommands(const QString &iface, QString *error) const;
  QStringList buildTasCommands(const QString &iface, QString *error,
                               qint64 *baseTimeNs) const;
  bool runCommand(const QString &command, QString *error) const;
  bool runCommands(const QStringList &commands, QString *error) const;
  bool clearExistingQdiscIfNeeded(const QString &iface, QString *error) const;
  bool startTerminalScript(const QString &script, QString *error) const;
  bool startSystemSyncInternal(const QString &iface, bool isSlave, QString *error);
  QJsonObject buildSchemeJson() const;
  bool applySchemeJson(const QJsonObject &obj, QString *error);
  bool stopTimeSyncInternal(bool showMessage);
  bool isPtp4lRunning() const;
  QString currentPhcDevice() const;
  void refreshPhcTimeInternal(bool showError);

  QString phcDevicePath_;
  QComboBox *phyIface_ = nullptr;
  QLineEdit *phyIpCidr_ = nullptr;
  QSpinBox *vlanId_ = nullptr;
  QLineEdit *vlanIface_ = nullptr;
  QLineEdit *vlanIpCidr_ = nullptr;
  QPushButton *btnIfaces_ = nullptr;
  QPushButton *btnInitApply_ = nullptr;
  QPushButton *btnRoleMaster_ = nullptr;
  QPushButton *btnRoleBackup_ = nullptr;
  QPushButton *btnRoleSlave_ = nullptr;
  QPushButton *btnStartTimeSync_ = nullptr;
  QPushButton *btnSwitchRole_ = nullptr;
  QPushButton *btnStopTimeSync_ = nullptr;
  QPushButton *btnSaveScheme_ = nullptr;
  QPushButton *btnLoadScheme_ = nullptr;
  QLabel *syncAccuracyLabel_ = nullptr;
  QLabel *systemSyncAccuracyLabel_ = nullptr;
  QLabel *phcTimeLabel_ = nullptr;
  QPushButton *btnRefreshPhcTime_ = nullptr;
  QLineEdit *phcAdjustSecondsEdit_ = nullptr;
  QPushButton *btnApplyPhcAdjust_ = nullptr;
  QTimer *phcRefreshTimer_ = nullptr;
  QRadioButton *cbsRadio_ = nullptr;
  QRadioButton *tasRadio_ = nullptr;
  QStackedWidget *shapeStack_ = nullptr;
  QTableWidget *cbsTable_ = nullptr;
  QSpinBox *tasDelay_ = nullptr;
  QTableWidget *tasTable_ = nullptr;
  QPushButton *btnShapeApply_ = nullptr;
  QLabel *gclBaseTimeLabel_ = nullptr;

  bool vlanIfaceManual_ = false;
  bool systemSyncScheduled_ = false;
  bool timeSyncActive_ = false;
  bool suppressRoleRestart_ = false;
  QString currentRole_ = "backup";
  QString lastPtpRoleFromLog_ = "unknown";
  QString lastOffsetFromLog_;
  QString lastSystemOffsetFromLog_;
  QProcess *ptpProcess_ = nullptr;
  QProcess *phcProcess_ = nullptr;
  QString ptpBuffer_;
  QString phcBuffer_;
};
