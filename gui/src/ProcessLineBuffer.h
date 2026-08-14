#pragma once

#include <QByteArray>
#include <QStringList>

class ProcessLineBuffer final {
 public:
  QStringList append(const QByteArray &chunk);
  QStringList flush();

 private:
  QByteArray buffer_;
};
