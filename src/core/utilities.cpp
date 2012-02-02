/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "timeconstants.h"
#include "utilities.h"
#include "core/logging.h"

#include "sha2.h"

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QIODevice>
#include <QMouseEvent>
#include <QStringList>
#include <QTcpServer>
#include <QTemporaryFile>
#include <QUrl>
#include <QWidget>
#include <QXmlStreamReader>
#include <QtDebug>
#include <QtGlobal>
#include <QMetaEnum>

#if defined(Q_OS_UNIX)
#  include <sys/statvfs.h>
#elif defined(Q_OS_WIN32)
#  include <windows.h>
#endif

#ifdef Q_OS_DARWIN
#  include "core/mac_startup.h"
#  include "CoreServices/CoreServices.h"
#endif

#include <boost/scoped_array.hpp>

namespace Utilities {

static QString tr(const char* str) {
  return QCoreApplication::translate("", str);
}

QString PrettyTimeDelta(int seconds) {
  return (seconds >= 0 ? "+" : "-") + PrettyTime(seconds);
}

QString PrettyTime(int seconds) {
  // last.fm sometimes gets the track length wrong, so you end up with
  // negative times.
  seconds = qAbs(seconds);

  int hours = seconds / (60*60);
  int minutes = (seconds / 60) % 60;
  seconds %= 60;

  QString ret;
  if (hours)
    ret.sprintf("%d:%02d:%02d", hours, minutes, seconds);
  else
    ret.sprintf("%d:%02d", minutes, seconds);

  return ret;
}

QString PrettyTimeNanosec(qint64 nanoseconds) {
  return PrettyTime(nanoseconds / kNsecPerSec);
}

QString WordyTime(quint64 seconds) {
  quint64 days = seconds / (60*60*24);

  // TODO: Make the plural rules translatable
  QStringList parts;

  if (days)
    parts << (days == 1 ? tr("1 day") : tr("%1 days").arg(days));
  parts << PrettyTime(seconds - days*60*60*24);

  return parts.join(" ");
}

QString WordyTimeNanosec(qint64 nanoseconds) {
  return WordyTime(nanoseconds / kNsecPerSec);
}

QString Ago(int seconds_since_epoch, const QLocale& locale) {
  const QDateTime now = QDateTime::currentDateTime();
  const QDateTime then = QDateTime::fromTime_t(seconds_since_epoch);
  const int days_ago = then.date().daysTo(now.date());
  const QString time = then.time().toString(locale.timeFormat(QLocale::ShortFormat));

  if (days_ago == 0)
    return tr("Today") + " " + time;
  if (days_ago == 1)
    return tr("Yesterday") + " " + time;
  if (days_ago <= 7)
    return tr("%1 days ago").arg(days_ago);

  return then.date().toString(locale.dateFormat());
}

QString PrettySize(quint64 bytes) {
  QString ret;

  if (bytes > 0) {
    if (bytes <= 1000)
      ret = QString::number(bytes) + " bytes";
    else if (bytes <= 1000*1000)
      ret.sprintf("%.1f KB", float(bytes) / 1000);
    else if (bytes <= 1000*1000*1000)
      ret.sprintf("%.1f MB", float(bytes) / (1000*1000));
    else
      ret.sprintf("%.1f GB", float(bytes) / (1000*1000*1000));
  }
  return ret;
}

quint64 FileSystemCapacity(const QString& path) {
#if defined(Q_OS_UNIX)
  struct statvfs fs_info;
  if (statvfs(path.toLocal8Bit().constData(), &fs_info) == 0)
    return quint64(fs_info.f_blocks) * quint64(fs_info.f_bsize);
#elif defined(Q_OS_WIN32)
  _ULARGE_INTEGER ret;
  if (GetDiskFreeSpaceEx(QDir::toNativeSeparators(path).toLocal8Bit().constData(),
                         NULL, &ret, NULL) != 0)
    return ret.QuadPart;
#endif

  return 0;
}

quint64 FileSystemFreeSpace(const QString& path) {
#if defined(Q_OS_UNIX)
  struct statvfs fs_info;
  if (statvfs(path.toLocal8Bit().constData(), &fs_info) == 0)
    return quint64(fs_info.f_bavail) * quint64(fs_info.f_bsize);
#elif defined(Q_OS_WIN32)
  _ULARGE_INTEGER ret;
  if (GetDiskFreeSpaceEx(QDir::toNativeSeparators(path).toLocal8Bit().constData(),
                         &ret, NULL, NULL) != 0)
    return ret.QuadPart;
#endif

  return 0;
}

QString MakeTempDir(const QString template_name) {
  QString path;
  {
    QTemporaryFile tempfile;
    if (!template_name.isEmpty())
      tempfile.setFileTemplate(template_name);

    tempfile.open();
    path = tempfile.fileName();
  }

  QDir d;
  d.mkdir(path);

  return path;
}

void RemoveRecursive(const QString& path) {
  QDir dir(path);
  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Hidden))
    RemoveRecursive(path + "/" + child);

  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Hidden))
    QFile::remove(path + "/" + child);

  dir.rmdir(path);
}

bool CopyRecursive(const QString& source, const QString& destination) {
  // Make the destination directory
  QString dir_name = source.section('/', -1, -1);
  QString dest_path = destination + "/" + dir_name;
  QDir().mkpath(dest_path);

  QDir dir(source);
  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs)) {
    if (!CopyRecursive(source + "/" + child, dest_path)) {
      qLog(Warning) << "Failed to copy dir" << source + "/" + child << "to" << dest_path;
      return false;
    }
  }

  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Files)) {
    if (!QFile::copy(source + "/" + child, dest_path + "/" + child)) {
      qLog(Warning) << "Failed to copy file" << source + "/" + child << "to" << dest_path;
      return false;
    }
  }
  return true;
}

bool Copy(QIODevice* source, QIODevice* destination) {
  if (!source->open(QIODevice::ReadOnly))
    return false;

  if (!destination->open(QIODevice::WriteOnly))
    return false;

  const qint64 bytes = source->size();
  boost::scoped_array<char> data(new char[bytes]);
  qint64 pos = 0;

  qint64 bytes_read;
  do {
    bytes_read = source->read(data.get() + pos, bytes - pos);
    if (bytes_read == -1)
      return false;

    pos += bytes_read;
  } while (bytes_read > 0 && pos != bytes);

  pos = 0;
  qint64 bytes_written;
  do {
    bytes_written = destination->write(data.get() + pos, bytes - pos);
    if (bytes_written == -1)
      return false;

    pos += bytes_written;
  } while (bytes_written > 0 && pos != bytes);

  return true;
}

QString ColorToRgba(const QColor& c) {
  return QString("rgba(%1, %2, %3, %4)")
      .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

QString GetConfigPath(ConfigPath config) {
  switch (config) {
    case Path_Root: {
      #ifdef Q_OS_DARWIN
        return mac::GetApplicationSupportPath() + "/" + QCoreApplication::organizationName();
      #else
        return QString("%1/.config/%2").arg(QDir::homePath(), QCoreApplication::organizationName());
      #endif
    }
    break;

    case Path_AlbumCovers:
      return GetConfigPath(Path_Root) + "/albumcovers";

    case Path_NetworkCache:
      return GetConfigPath(Path_Root) + "/networkcache";

    case Path_GstreamerRegistry:
      return GetConfigPath(Path_Root) +
          QString("/gst-registry-%1-bin").arg(QCoreApplication::applicationVersion());

    case Path_DefaultMusicLibrary:
      #ifdef Q_OS_DARWIN
        return mac::GetMusicDirectory();
      #else
        return QDir::homePath();
      #endif

    case Path_LocalSpotifyBlob:
      return GetConfigPath(Path_Root) + "/spotifyblob";

    case Path_SongCache:
      return GetConfigPath(Path_Root) + "/songcache";

    default:
      qFatal("%s", Q_FUNC_INFO);
      return QString::null;
  }
}

#ifdef Q_OS_DARWIN
qint32 GetMacVersion() {
  SInt32 minor_version;
  Gestalt(gestaltSystemVersionMinor, &minor_version);
  return minor_version;
}
#endif  // Q_OS_DARWIN

void OpenInFileBrowser(const QList<QUrl>& urls) {
  QSet<QString> dirs;

  foreach (const QUrl& url, urls) {
    if (url.scheme() != "file") {
      continue;
    }
    QString path = url.toLocalFile();

    if (!QFile::exists(path))
      continue;

    const QString directory = QFileInfo(path).dir().path();

    if (dirs.contains(directory))
      continue;
    dirs.insert(directory);

    QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
  }
}

QByteArray Hmac(const QByteArray& key, const QByteArray& data, HashFunction method) {
  const int kBlockSize = 64; // bytes
  Q_ASSERT(key.length() <= kBlockSize);

  QByteArray inner_padding(kBlockSize, char(0x36));
  QByteArray outer_padding(kBlockSize, char(0x5c));

  for (int i=0 ; i<key.length() ; ++i) {
    inner_padding[i] = inner_padding[i] ^ key[i];
    outer_padding[i] = outer_padding[i] ^ key[i];
  }
  if (Md5_Algo == method) {
    return QCryptographicHash::hash(outer_padding +
                                    QCryptographicHash::hash(inner_padding + data,
                                                             QCryptographicHash::Md5),
                                    QCryptographicHash::Md5);
  } else { // Sha256_Algo, currently default
    return Sha256(outer_padding + Sha256(inner_padding + data));
  }
}

QByteArray HmacSha256(const QByteArray& key, const QByteArray& data) {
  return Hmac(key, data, Sha256_Algo);
}

QByteArray HmacMd5(const QByteArray& key, const QByteArray& data) {
  return Hmac(key, data, Md5_Algo);
}

QByteArray Sha256(const QByteArray& data) {
  SHA256_CTX context;
  SHA256_Init(&context);
  SHA256_Update(&context, reinterpret_cast<const u_int8_t*>(data.constData()),
                data.length());

  QByteArray ret(SHA256_DIGEST_LENGTH, '\0');
  SHA256_Final(reinterpret_cast<u_int8_t*>(ret.data()), &context);

  return ret;
}

QString PrettySize(const QSize& size) {
  return QString::number(size.width()) + "x" +
         QString::number(size.height());
}

void ForwardMouseEvent(const QMouseEvent* e, QWidget* target) {
  QMouseEvent c(e->type(), target->mapFromGlobal(e->globalPos()),
                e->globalPos(), e->button(), e->buttons(), e->modifiers());

  QApplication::sendEvent(target, &c);
}

bool IsMouseEventInWidget(const QMouseEvent* e, const QWidget* widget) {
  return widget->rect().contains(widget->mapFromGlobal(e->globalPos()));
}

quint16 PickUnusedPort() {
  forever {
    const quint16 port = 49152 + qrand() % 16384;

    QTcpServer server;
    if (server.listen(QHostAddress::Any, port)) {
      return port;
    }
  }
}

void ConsumeCurrentElement(QXmlStreamReader* reader) {
  int level = 1;
  while (level != 0 && !reader->atEnd()) {
    switch (reader->readNext()) {
      case QXmlStreamReader::StartElement: ++level; break;
      case QXmlStreamReader::EndElement:   --level; break;
      default: break;
    }
  }
}

const char* EnumToString(const QMetaObject& meta, const char* name, int value) {
  int index = meta.indexOfEnumerator(name);
  if (index == -1)
    return "[UnknownEnum]";
  QMetaEnum metaenum = meta.enumerator(index);
  const char* result = metaenum.valueToKey(value);
  if (result == 0)
    return "[UnknownEnumValue]";
  return result;
}

}  // namespace Utilities


ScopedWCharArray::ScopedWCharArray(const QString& str)
  : chars_(str.length()),
    data_(new wchar_t[chars_ + 1])
{
  str.toWCharArray(data_.get());
  data_[chars_] = '\0';
}
