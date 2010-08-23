/* This file is part of Clementine.

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

#ifndef WMDMDEVICE_H
#define WMDMDEVICE_H

#include "connecteddevice.h"

class WmdmLoader;

class WmdmDevice : public ConnectedDevice {
  Q_OBJECT

public:
  Q_INVOKABLE WmdmDevice(const QUrl& url, DeviceLister* lister,
                         const QString& unique_id, DeviceManager* manager,
                         int database_id, bool first_time);
  ~WmdmDevice();

  static QStringList url_schemes() { return QStringList() << "wmdm"; }

  void Init();

  void StartCopy();
  bool CopyToStorage(const QString& source, const QString& destination,
                     const Song& metadata, bool overwrite, bool remove_original);
  void FinishCopy(bool success);

  void StartDelete();
  bool DeleteFromStorage(const Song& metadata);
  void FinishDelete(bool success);

private slots:
  void LoadFinished();

private:
  QThread* loader_thread_;
  WmdmLoader* loader_;

  QMutex db_busy_;
  SongList songs_to_add_;
  SongList songs_to_remove_;
};

#endif // WMDMDEVICE_H