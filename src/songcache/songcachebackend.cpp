#include "songcachebackend.h"

SongCacheBackend::SongCacheBackend(boost::shared_ptr<Database> db, QObject* parent)
  : QObject(parent),
    db_(db)
{
}
