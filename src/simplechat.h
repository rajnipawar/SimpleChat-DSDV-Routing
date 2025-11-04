#pragma once

#include <QObject>
#include <QTimer>
#include "chatwindow.h"
#include "networkmanager.h"
#include "message.h"

class SimpleChat : public QObject {
    Q_OBJECT

public:
    explicit SimpleChat(int port, const QList<int>& peerPorts = QList<int>(), bool noForwardMode = false, QObject* parent = nullptr);
    ~SimpleChat();

    void show();

private slots:
    void onMessageEntered(const QString& text, const QString& destination);
    void onMessageReceived(const Message& message);
    void onPeerDiscovered(const QString& peerId, const QString& host, int port);
    void onPeerStatusChanged(const QString& peerId, bool active);
    void onAddPeerRequested(const QString& host, int port);

private:
    QString generateNodeId(int port);
    void setupPeerDiscovery();

    ChatWindow* window;
    NetworkManager* networkManager;
    int serverPort;
    QString nodeId;
    QList<int> discoveryPorts;

    static const QList<int> DEFAULT_PORTS;
};
