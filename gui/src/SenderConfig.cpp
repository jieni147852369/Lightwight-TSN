#include "SenderConfig.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

static QString trimmed(const QString &s) { return s.trimmed(); }

static bool parseKeyValue(const QString &line, QString *key, QString *value) {
  const int eq = line.indexOf('=');
  if (eq < 0) return false;
  if (key) *key = trimmed(line.left(eq));
  if (value) *value = trimmed(line.mid(eq + 1));
  return true;
}

static bool isCommentOrEmpty(const QString &line) {
  const QString t = trimmed(line);
  return t.isEmpty() || t.startsWith('#');
}

bool SenderConfigIo::loadFromFile(const QString &path, SenderConfig *out, QString *error) {
  if (!out) {
    if (error) *error = "internal: out is null";
    return false;
  }

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (error) *error = QString("无法打开配置文件：%1").arg(path);
    return false;
  }

  SenderConfig cfg;
  cfg.frames.clear();
  SenderFrame *current = nullptr;

  QTextStream in(&f);
  int lineNo = 0;
  while (!in.atEnd()) {
    lineNo++;
    const QString raw = in.readLine();
    if (isCommentOrEmpty(raw)) continue;
    const QString t = trimmed(raw);

    if (t.startsWith('[')) {
      const int close = t.indexOf(']');
      if (close < 0) {
        if (error) *error = QString("配置文件第%1行缺少 ']'").arg(lineNo);
        return false;
      }
      const QString section = trimmed(t.mid(1, close - 1));
      if (!section.toLower().startsWith("frame")) {
        if (error) *error = QString("配置文件第%1行未知 section：%2").arg(lineNo).arg(section);
        return false;
      }
      QString label = trimmed(section.mid(5));
      if (label.isEmpty()) label = QString("frame%1").arg(cfg.frames.size() + 1);
      SenderFrame frame;
      frame.name = label;
      cfg.frames.push_back(frame);
      current = &cfg.frames.back();
      continue;
    }

    QString key, value;
    if (!parseKeyValue(t, &key, &value)) {
      if (error) *error = QString("配置文件第%1行格式错误：%2").arg(lineNo).arg(t);
      return false;
    }

    if (key.compare("interface", Qt::CaseInsensitive) == 0) {
      cfg.interface = value;
      continue;
    }
    if (key.compare("base_start_ns", Qt::CaseInsensitive) == 0 ||
        key.compare("start_time_ns", Qt::CaseInsensitive) == 0) {
      bool ok = false;
      const quint64 v = value.toULongLong(&ok, 0);
      if (!ok) {
        if (error) *error = QString("base_start_ns 无法解析（第%1行）").arg(lineNo);
        return false;
      }
      cfg.baseStartNs = v;
      continue;
    }
    if (key.compare("warmup_ms", Qt::CaseInsensitive) == 0) {
      bool ok = false;
      const quint64 v = value.toULongLong(&ok, 0);
      if (!ok) {
        if (error) *error = QString("warmup_ms 无法解析（第%1行）").arg(lineNo);
        return false;
      }
      cfg.warmupMs = v;
      continue;
    }

    if (!current) {
      if (error) *error = QString("配置文件第%1行：frame 选项出现在 [frame] 之前").arg(lineNo);
      return false;
    }

    if (key.compare("dst_ip", Qt::CaseInsensitive) == 0) {
      current->dstIp = value;
    } else if (key.compare("dst_port", Qt::CaseInsensitive) == 0) {
      current->dstPort = value.toInt();
    } else if (key.compare("udp_src_port", Qt::CaseInsensitive) == 0) {
      if (value.isEmpty()) {
        current->udpSrcPort.reset();
      } else {
        current->udpSrcPort = value.toInt();
      }
    } else if (key.compare("frame_count", Qt::CaseInsensitive) == 0) {
      bool ok = false;
      const quint64 cnt = value.toULongLong(&ok, 0);
      if (!ok) {
        if (error) *error = QString("frame_count 无法解析（第%1行）").arg(lineNo);
        return false;
      }
      current->frameCount = cnt;
    } else if (key.compare("payload_len", Qt::CaseInsensitive) == 0) {
      current->payloadLen = value.toInt();
    } else if (key.compare("period_us", Qt::CaseInsensitive) == 0) {
      bool ok = false;
      const quint64 us = value.toULongLong(&ok, 0);
      if (!ok) {
        if (error) *error = QString("period_us 无法解析（第%1行）").arg(lineNo);
        return false;
      }
      current->periodUs = us;
    } else if (key.compare("socket_priority", Qt::CaseInsensitive) == 0) {
      current->socketPriority = value.toInt();
    } else {
      // Ignore unknown keys for forward compatibility.
    }
  }

  if (cfg.interface.isEmpty()) {
    if (error) *error = "interface 不能为空";
    return false;
  }
  if (cfg.frames.empty()) {
    if (error) *error = "至少需要一个 [frame]";
    return false;
  }
  for (const auto &fr : cfg.frames) {
    if (fr.dstIp.isEmpty() || fr.dstPort <= 0 || fr.payloadLen <= 0 || fr.periodUs == 0) {
      if (error) *error = QString("frame [%1] 配置不完整").arg(fr.name);
      return false;
    }
  }

  *out = cfg;
  return true;
}

bool SenderConfigIo::saveToFile(const QString &path, const SenderConfig &cfg, QString *error) {
  if (cfg.interface.isEmpty()) {
    if (error) *error = "interface 不能为空";
    return false;
  }
  if (cfg.frames.empty()) {
    if (error) *error = "至少需要一个 [frame]";
    return false;
  }

  const QString tmpPath = path + ".tmp";
  QFile f(tmpPath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    if (error) *error = QString("无法写入配置文件：%1").arg(path);
    return false;
  }

  QTextStream out(&f);
  out << "# Auto-generated by TSN_SoftWare\n\n";
  out << "interface=" << cfg.interface << "\n";
  out << "base_start_ns=" << cfg.baseStartNs << "\n";
  out << "warmup_ms=" << cfg.warmupMs << "\n\n";

  for (const auto &fr : cfg.frames) {
    out << "[frame " << fr.name << "]\n";
    out << "dst_ip=" << fr.dstIp << "\n";
    out << "dst_port=" << fr.dstPort << "\n";
    if (fr.udpSrcPort.has_value()) out << "udp_src_port=" << *fr.udpSrcPort << "\n";
    if (fr.frameCount.has_value()) out << "frame_count=" << *fr.frameCount << "\n";
    out << "payload_len=" << fr.payloadLen << "\n";
    out << "period_us=" << fr.periodUs << "\n";
    out << "socket_priority=" << fr.socketPriority << "\n\n";
  }
  f.close();

  QFile::remove(path);
  if (!QFile::rename(tmpPath, path)) {
    QFile::remove(tmpPath);
    if (error) *error = QString("保存失败：无法替换文件 %1").arg(path);
    return false;
  }
  return true;
}
