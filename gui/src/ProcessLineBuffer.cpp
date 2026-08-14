#include "ProcessLineBuffer.h"

QStringList ProcessLineBuffer::append(const QByteArray &chunk) {
  buffer_.append(chunk);
  return flush();
}

QStringList ProcessLineBuffer::flush() {
  QStringList out;
  for (;;) {
    const int nl = buffer_.indexOf('\n');
    if (nl < 0) break;
    QByteArray line = buffer_.left(nl);
    buffer_.remove(0, nl + 1);
    if (!line.isEmpty() && line.endsWith('\r')) line.chop(1);
    out.push_back(QString::fromLocal8Bit(line));
  }
  return out;
}
