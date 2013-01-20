#ifndef NETWORKREMOTE_H
#define NETWORKREMOTE_H

#include <boost/scoped_ptr.hpp>

#include <QTcpServer>
#include <QTcpSocket>

#include "core/player.h"
#include "core/application.h"
#include "incomingdataparser.h"
#include "outgoingdatacreator.h"
#include "remoteclient.h"

class NetworkRemote : public QObject {
    Q_OBJECT
public:
  static const char* kSettingsGroup;
  static const quint16 kDefaultServerPort;
  static const int kProtocolBufferVersion;

  explicit NetworkRemote(Application* app, QObject* parent = 0);
  ~NetworkRemote();

public slots:
  void SetupServer();
  void StartServer();
  void ReloadSettings();
  void AcceptConnection();

private:
  boost::scoped_ptr<QTcpServer> server_;
  boost::scoped_ptr<QTcpServer> server_ipv6_;
  boost::scoped_ptr<IncomingDataParser> incoming_data_parser_;
  boost::scoped_ptr<OutgoingDataCreator> outgoing_data_creator_;

  quint16 port_;
  bool use_remote_;
  bool only_non_public_ip_;
  bool signals_connected_;
  Application* app_;

  QList<RemoteClient*> clients_;

  void StopServer();
  void ReadSettings();
  void CreateRemoteClient(QTcpSocket* client_socket);
  bool IpIsPrivate(const QHostAddress& address);
};

#endif // NETWORKREMOTE_H
