#ifndef SONGCACHEBACKEND_H
#define SONGCACHEBACKEND_H

#include <QObject>

#include <boost/shared_ptr.hpp>

class QNetworkAccessManager;

class Database;

class SongCacheBackend : public QObject
{
  Q_OBJECT

 public:
  SongCacheBackend(boost::shared_ptr<Database> db, QObject* parent = 0);
  virtual ~SongCacheBackend() {}

  struct CacheFile {
    CacheFile() {}
    CacheFile(const QString& _id, int _size, int _mtime,
              int _lastused, bool _complete, bool _pinned)
      : id(_id), size(_size), mtime(_mtime),
        lastused(_lastused), complete(_complete), pinned(_pinned) {}

    // Unique cache file ID - code using the SongCache should generate this from
    // a combination of service name, a unique identifier within that service, and
    // any other information required to disambiguate. The ID must also be filename
    // safe. Example: subsonic@d3b07384d113edec49eaa6238ad5ff00_c157a79031e1c40f85931829bc5fc552
    QString id;
    // File size - compared to on-disk size for download progress
    int size;
    // Preferably modified time, but could by any value where a change in the value
    // should invalidate the cache for the song
    int mtime;
    // When the cache file was last accessed - used for deciding which cache files to
    // remove when reclaiming space
    int lastused;
    // Should be true when the file has been downloaded completely - helps with pruning
    // incomplete cache files without hammering the filesystem
    bool complete;
    // Mark this cache file as persistent - it will be ignored when reclaiming space
    bool pinned;
  };

 private:
  boost::shared_ptr<Database> db_;
  QNetworkAccessManager *network_;
};

#endif // SONGCACHEBACKEND_H
