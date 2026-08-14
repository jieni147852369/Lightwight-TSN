#pragma once

#include <QQueue>
#include <QProcess>
#include <QStringList>
#include <QWidget>

#include "ProcessLineBuffer.h"
#include "ReceiverConfig.h"

class QComboBox;
class QFile;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;

class ReceiverWidget final : public QWidget {
  Q_OBJECT

 public:
  explicit ReceiverWidget(QWidget *parent = nullptr);

 private slots:
  void browseConfig();
  void loadConfig();
  void saveConfig();
  void saveConfigAs();

  void reloadInterfaces();
  void startReceiver();
  void stopReceiver();
  void clearOutput();
  void exportOutputTxt();

  void onStdout();
  void onStderr();
  void onFinished(int exitCode, QProcess::ExitStatus status);
  void flushPendingRows();

 private:
  QString resolveDefaultConfigPath() const;
  QString resolveReceiverBinary() const;

  ReceiverConfig configFromUi(QString *error) const;
  bool fillUiFromConfig(const ReceiverConfig &cfg);

  void appendReceiverLine(const QString &line);
  void addOutputRow(const QStringList &cols);
  void updateStatsUi();
  void flushLogBuffer();
  void trimOutputRows();

  QLineEdit *configPath_ = nullptr;
  QPushButton *btnBrowse_ = nullptr;
  QPushButton *btnOpen_ = nullptr;
  QPushButton *btnSave_ = nullptr;
  QPushButton *btnSaveAs_ = nullptr;

  QComboBox *iface_ = nullptr;
  QPushButton *btnReloadIfaces_ = nullptr;
  QSpinBox *port_ = nullptr;

  QPushButton *btnStart_ = nullptr;
  QPushButton *btnStop_ = nullptr;
  QPushButton *btnClear_ = nullptr;
  QPushButton *btnExport_ = nullptr;

  QLabel *stats_ = nullptr;
  QLabel *logPath_ = nullptr;

  QTableWidget *output_ = nullptr;
  int maxOutputRows_ = 5000;

  QProcess *proc_ = nullptr;
  ProcessLineBuffer stdoutBuf_;
  ProcessLineBuffer stderrBuf_;

  QTimer *flushTimer_ = nullptr;
  QQueue<QStringList> pendingRows_;
  int maxRowsPerFlush_ = 300;

  quint64 totalMatched_ = 0;
  quint64 droppedUi_ = 0;
  QString lastSeq_;
  bool statsDirty_ = true;

  QString logFilePath_;
  QFile *logFile_ = nullptr;
  QByteArray logWriteBuffer_;
};
