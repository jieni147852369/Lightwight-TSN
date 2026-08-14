#pragma once

#include <optional>
#include <vector>

#include <QString>

struct SenderFrame final {
  QString name;
  QString dstIp;
  int dstPort = 0;
  std::optional<int> udpSrcPort;
  std::optional<quint64> frameCount;
  int payloadLen = 0;
  quint64 periodUs = 0;
  int socketPriority = 0;
};

struct SenderConfig final {
  QString interface;
  quint64 baseStartNs = 0;  // 0 means start now.
  quint64 warmupMs = 500;
  std::vector<SenderFrame> frames;
};

class SenderConfigIo final {
 public:
  static bool loadFromFile(const QString &path, SenderConfig *out, QString *error);
  static bool saveToFile(const QString &path, const SenderConfig &cfg, QString *error);
};
