#pragma once

#include <QQueue>
#include <QProcess>
#include <QTimer>
#include <QWidget>

#include <memory>
#include <vector>

#include "ProcessLineBuffer.h"
#include "SenderConfig.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QFile;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTableWidget;

class SenderWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit SenderWidget(QWidget *parent = nullptr);

 private slots:
  void browseConfig();
  void loadConfig();
  void saveConfig();
  void saveConfigAs();

  void addFrame();
  void removeSelectedFrames();

  void startSender();
  void startSenderMulti();
  void stopSender();
  void clearOutput();
  void exportOutputTxt();

  void reloadInterfaces();
  void updateVlanDerived();  // reserved, not used for sender

  void onTick();
  void onFinished(int exitCode, QProcess::ExitStatus status);

 private:
  enum class StartMode { Now, AbsoluteNs, DelaySeconds };

  struct SenderProcessContext {
    QString name;
    QString configPath;
    bool deleteConfigOnExit = false;
    QProcess *proc = nullptr;
    QString logFilePath;
    ProcessLineBuffer logBuf;
    QFile *liveLogWriter = nullptr;
  };

  QString resolveDefaultConfigPath() const;
  QString resolveSenderBinary() const;
  QString makeLogFilePath(const QString &label);
  QString makeTempConfigPath(const QString &label, int index) const;

  SenderConfig configFromUi(QString *error, bool computeDelayToAbsolute) const;
  bool fillUiFromConfig(const SenderConfig &cfg, QString *error);
  bool ensureReadyToStart(QString *error) const;
  void resetForNewRun();

  bool startSenderProcess(const QString &bin, const QString &configPath, const QString &label,
                          bool deleteConfigOnExit, QString *error);
  SenderProcessContext *findContext(QProcess *proc);
  void handleProcessOutput(SenderProcessContext *ctx, bool isStdout);

  void appendSenderLine(const QString &line);
  void flushPendingRows();
  void trimOutputRows();
  void cleanupContext(SenderProcessContext *ctx);
  void updateButtons();
  bool hasRunningProcess() const;

  QLineEdit *configPath_ = nullptr;
  QPushButton *btnBrowse_ = nullptr;
  QPushButton *btnOpen_ = nullptr;
  QPushButton *btnSave_ = nullptr;
  QPushButton *btnSaveAs_ = nullptr;

  QComboBox *iface_ = nullptr;
  QPushButton *btnReloadIfaces_ = nullptr;

  QRadioButton *rbNow_ = nullptr;
  QRadioButton *rbAbs_ = nullptr;
  QRadioButton *rbDelay_ = nullptr;
  QLineEdit *absNs_ = nullptr;
  QDoubleSpinBox *delaySec_ = nullptr;
  QSpinBox *warmupMs_ = nullptr;

  QTableWidget *frames_ = nullptr;
  QPushButton *btnAddFrame_ = nullptr;
  QPushButton *btnRemoveFrame_ = nullptr;

  QPushButton *btnStart_ = nullptr;
  QPushButton *btnStartMulti_ = nullptr;
  QPushButton *btnStop_ = nullptr;
  QPushButton *btnClear_ = nullptr;
  QPushButton *btnExport_ = nullptr;

  QTableWidget *output_ = nullptr;
  int maxOutputRows_ = 5000;
  int maxRowsPerFlush_ = 200;

  QTimer *tickTimer_ = nullptr;
  QQueue<QStringList> pendingRows_;
  int droppedUi_ = 0;

  std::vector<std::unique_ptr<SenderProcessContext>> processes_;
  QStringList lastLogFiles_;
  quint64 logFileCounter_ = 0;
};
