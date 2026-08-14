#pragma once

#include <QString>

struct ReceiverConfig final {
  QString interface;
  int port = 0;
};

class ReceiverConfigIo final {
 public:
  static bool loadFromFile(const QString &path, ReceiverConfig *out, QString *error);
  static bool saveToFile(const QString &path, const ReceiverConfig &cfg, QString *error);
};

