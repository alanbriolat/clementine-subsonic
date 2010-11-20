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

#ifndef QUERYPLAYLISTGENERATOR_H
#define QUERYPLAYLISTGENERATOR_H

#include "generator.h"
#include "search.h"

namespace smart_playlists {

class QueryGenerator : public Generator {
public:
  QueryGenerator();

  QString type() const { return "Query"; }

  void Load(const Search& search);
  void Load(const QByteArray& data);
  QByteArray Save() const;

  PlaylistItemList Generate();

  Search search() const { return search_; }

private:
  Search search_;
};

} // namespace

#endif // QUERYPLAYLISTGENERATOR_H