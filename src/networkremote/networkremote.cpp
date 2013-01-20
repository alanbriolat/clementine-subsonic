/* This file is part of Clementine.
   Copyright 2012, Andreas Muttscheller <asfa194@gmail.com>

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

#include "networkremote.h"

#include "core/logging.h"
#include "covers/currentartloader.h"
#include "networkremote/zeroconf.h"
#include "playlist/playlistmanager.h"


#include <QDataStream>
#include <QSettings>

const char* NetworkRemote::kSettingsGroup = "NetworkRemote";
const quint16 NetworkRemote::kDefaultServerPort = 5500;
const int NetworkRemote::kProtocolBufferVersion = 1;

NetworkRemote::NetworkRemote(Application* app, QObject* parent)
  : QObject(parent),
    signals_connected_(false),
    app_(app) {
}


NetworkRemote::~NetworkRemote() {
  StopServer();
}

void NetworkRemote::ReadSettings() {
  QSettings s;

  s.beginGroup(NetworkRemote::kSettingsGroup);
  use_remote_ = s.value("use_remote", false).toBool();
  port_       = s.value("port", kDefaultServerPort).toInt();

  // Use only non public ips must be true be default
  only_non_public_ip_ = s.value("only_non_public_ip", true).toBool();

  s.endGroup();
}

void NetworkRemote::SetupServer() {
  server_.reset(new QTcpServer());
  server_ipv6_.reset(new QTcpServer());
  incoming_data_parser_.reset(new IncomingDataParser(app_));
  outgoing_data_creator_.reset(new OutgoingDataCreator(app_));

  outgoing_data_creator_->SetClients(&clients_);

  connect(app_->current_art_loader(),
          SIGNAL(ArtLoaded(const Song&, const QString&, const QImage&)),
          outgoing_data_creator_.get(),
          SLOT(CurrentSongChanged(const Song&, const QString&, const QImage&)));
}

void NetworkRemote::StartServer() {
  if (!app_) {
    qLog(Error) << "Start Server called without having an application!";
    return;
  }
  // Check if user desires to start a network remote server
  ReadSettings();
  if (!use_remote_) {
    qLog(Info) << "Network Remote deactivated";
    return;
  }

  qLog(Info) << "Starting network remote";

  connect(server_.get(), SIGNAL(newConnection()), this, SLOT(AcceptConnection()));
  connect(server_ipv6_.get(), SIGNAL(newConnection()), this, SLOT(AcceptConnection()));

  server_->listen(QHostAddress::Any, port_);
  server_ipv6_->listen(QHostAddress::AnyIPv6, port_);

  qLog(Info) << "Listening on port " << port_;

  if (Zeroconf::GetZeroconf()) {
    Zeroconf::GetZeroconf()->Publish(
        "local", "_clementine._tcp", "Clementine", port_);
  }
}

void NetworkRemote::StopServer() {
  if (server_->isListening()) {
    server_->close();
    server_ipv6_->close();
    clients_.clear();
  }
}

void NetworkRemote::ReloadSettings() {
  StopServer();
  StartServer();
}

void NetworkRemote::AcceptConnection() {
  if (!signals_connected_) {
    signals_connected_ = true;

    // Setting up the signals, but only once
    connect(incoming_data_parser_.get(), SIGNAL(SendClementineInfo()),
            outgoing_data_creator_.get(), SLOT(SendClementineInfo()));
    connect(incoming_data_parser_.get(), SIGNAL(SendFirstData()),
            outgoing_data_creator_.get(), SLOT(SendFirstData()));
    connect(incoming_data_parser_.get(), SIGNAL(SendAllPlaylists()),
            outgoing_data_creator_.get(), SLOT(SendAllPlaylists()));
    connect(incoming_data_parser_.get(), SIGNAL(SendPlaylistSongs(int)),
            outgoing_data_creator_.get(), SLOT(SendPlaylistSongs(int)));

    connect(app_->playlist_manager(), SIGNAL(ActiveChanged(Playlist*)),
            outgoing_data_creator_.get(), SLOT(ActiveChanged(Playlist*)));
    connect(app_->playlist_manager(), SIGNAL(PlaylistChanged(Playlist*)),
            outgoing_data_creator_.get(), SLOT(PlaylistChanged(Playlist*)));

    connect(app_->player(), SIGNAL(VolumeChanged(int)), outgoing_data_creator_.get(),
            SLOT(VolumeChanged(int)));
    connect(app_->player()->engine(), SIGNAL(StateChanged(Engine::State)),
            outgoing_data_creator_.get(), SLOT(StateChanged(Engine::State)));

    connect(app_->playlist_manager()->sequence(),
            SIGNAL(RepeatModeChanged(PlaylistSequence::RepeatMode)),
            outgoing_data_creator_.get(),
            SLOT(SendRepeatMode(PlaylistSequence::RepeatMode)));
    connect(app_->playlist_manager()->sequence(),
            SIGNAL(ShuffleModeChanged(PlaylistSequence::ShuffleMode)),
            outgoing_data_creator_.get(),
            SLOT(SendShuffleMode(PlaylistSequence::ShuffleMode)));
  }

  QTcpServer* server = qobject_cast<QTcpServer*>(sender());
  QTcpSocket* client_socket = server->nextPendingConnection();
  // Check if our ip is in private scope
  if (only_non_public_ip_ && !IpIsPrivate(client_socket->peerAddress())) {
    qLog(Info) << "Got a connection from public ip" <<
                client_socket->peerAddress().toString();
    client_socket->close();
  } else {
    CreateRemoteClient(client_socket);
  }
}

bool NetworkRemote::IpIsPrivate(const QHostAddress& address) {
  return
      // Link Local v4
      address.isInSubnet(QHostAddress::parseSubnet("127.0.0.1/8")) ||
      // Link Local v6
      address.isInSubnet(QHostAddress::parseSubnet("::1/128")) ||
      address.isInSubnet(QHostAddress::parseSubnet("fe80::/10")) ||
      // Private v4 range
      address.isInSubnet(QHostAddress::parseSubnet("192.168.0.0/16")) ||
      address.isInSubnet(QHostAddress::parseSubnet("172.16.0.0/12")) ||
      address.isInSubnet(QHostAddress::parseSubnet("10.0.0.0/8")) ||
      // Private v6 range
      address.isInSubnet(QHostAddress::parseSubnet("fc00::/7"));
}

void NetworkRemote::CreateRemoteClient(QTcpSocket *client_socket) {
  if (client_socket) {
    // Add the client to the list
    RemoteClient* client = new RemoteClient(app_, client_socket);
    clients_.push_back(client);

    // Connect the signal to parse data
    connect(client, SIGNAL(Parse(QByteArray)),
            incoming_data_parser_.get(), SLOT(Parse(QByteArray)));
  }
}
