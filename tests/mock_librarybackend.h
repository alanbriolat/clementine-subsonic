#ifndef MOCKLIBRARYBACKEND_H
#define MOCKLIBRARYBACKEND_H

#include <gmock/gmock.h>

#include "library/librarybackend.h"

class MockLibraryBackend : public LibraryBackendInterface {
 public:
  // Get a list of directories in the library.  Emits DirectoriesDiscovered.
  MOCK_METHOD0(LoadDirectoriesAsync, void());

  // Counts the songs in the library.  Emits TotalSongCountUpdated
  MOCK_METHOD0(UpdateTotalSongCountAsync, void());

  MOCK_METHOD1(FindSongsInDirectory, SongList(int));
  MOCK_METHOD1(SubdirsInDirectory, SubdirectoryList(int));
  MOCK_METHOD0(GetAllDirectories, DirectoryList());
  MOCK_METHOD2(ChangeDirPath, void(int, const QString&));

  MOCK_METHOD1(GetAllArtists, QStringList(const QueryOptions&));
  MOCK_METHOD1(GetAllArtistsWithAlbums, QStringList(const QueryOptions&));
  MOCK_METHOD3(GetSongs, SongList(const QString&, const QString&, const QueryOptions&));

  MOCK_METHOD1(HasCompilations, bool(const QueryOptions&));
  MOCK_METHOD2(GetCompilationSongs, SongList(const QString&, const QueryOptions&));

  MOCK_METHOD1(GetAllAlbums, AlbumList(const QueryOptions&));
  MOCK_METHOD2(GetAlbumsByArtist, AlbumList(const QString&, const QueryOptions&));
  MOCK_METHOD1(GetCompilationAlbums, AlbumList(const QueryOptions&));

  MOCK_METHOD3(UpdateManualAlbumArtAsync, void(const QString&, const QString&, const QString&));
  MOCK_METHOD2(GetAlbumArt, Album(const QString&, const QString&));

  MOCK_METHOD1(GetSongById, Song(int));

  MOCK_METHOD1(AddDirectory, void(const QString&));
  MOCK_METHOD1(RemoveDirectory, void(const Directory&));

  MOCK_METHOD1(ExecQuery, bool(LibraryQuery*));
};

#endif