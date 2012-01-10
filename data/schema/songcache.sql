/* Temporary schema for destroying and rebuilding the cache table
 * during development.
 *
 * XXX: This should eventually go into a schema update file.
 */
DROP INDEX IF EXISTS idx_songcache_id;
DROP TABLE IF EXISTS songcache;

CREATE TABLE songcache (
  id TEXT NOT NULL,
  size INTEGER NOT NULL,
  mtime INTEGER NOT NULL,
  lastused INTEGER NOT NULL,
  complete INTEGER NOT NULL DEFAULT 0,
  pinned INTEGER NOT NULL DEFAULT 0
);

CREATE UNIQUE INDEX idx_songcache_id ON songcache (id);
