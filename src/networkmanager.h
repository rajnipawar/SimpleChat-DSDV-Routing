#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <QPair>
#include <QDateTime>
#include "message.h"

struct PeerInfo {
    QString peerId;
    QString host;
    int port;
    bool isActive;
    qint64 lastSeen;

    PeerInfo() : port(0), isActive(false), lastSeen(0) {}
    PeerInfo(const QString& id, const QString& h, int p)
        : peerId(id), host(h), port(p), isActive(true), lastSeen(QDateTime::currentMSecsSinceEpoch()) {}
};

// DSDV Routing Table Entry
struct RouteInfo {
    QString nextHop;  // Next hop node ID
    QString nextHopIP;  // Next hop IP address
    quint16 nextHopPort;  // Next hop port
    int seqNo;  // Sequence number from origin
    bool isDirect;  // Is this a direct route?
    qint64 lastUpdated;  // Last time this route was updated

    RouteInfo() : nextHopPort(0), seqNo(0), isDirect(false), lastUpdated(0) {}
    RouteInfo(const QString& hop, const QString& ip, quint16 port, int seq, bool direct)
        : nextHop(hop), nextHopIP(ip), nextHopPort(port), seqNo(seq), isDirect(direct),
          lastUpdated(QDateTime::currentMSecsSinceEpoch()) {}
};

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    bool startServer(int port);
    void sendMessage(const Message& message);
    void addPeer(const QString& peerId, const QString& host, int port);
    void discoverLocalPeers(const QList<int>& portRange);

    void setNodeId(const QString& nodeId) { this->nodeId = nodeId; }
    QString getNodeId() const { return nodeId; }

    QList<QString> getActivePeers() const;
    QVariantMap getVectorClock() const { return vectorClock; }

    // PA3: Routing and noforward mode
    void setNoForwardMode(bool enabled) { noForwardMode = enabled; }
    bool isNoForwardMode() const { return noForwardMode; }
    QMap<QString, RouteInfo> getRoutingTable() const { return routingTable; }

signals:
    void messageReceived(const Message& message);
    void peerDiscovered(const QString& peerId, const QString& host, int port);
    void peerStatusChanged(const QString& peerId, bool active);

private slots:
    void onDataReceived();
    void onAntiEntropyTimeout();
    void checkPendingAcks();
    void checkPeerHealth();
    void sendRouteRumor();  // PA3: Send route rumors periodically

private:
    void processReceivedMessage(const Message& message, const QHostAddress& senderHost, quint16 senderPort);
    void handleChatMessage(const Message& message);
    void handleAntiEntropyRequest(const Message& message, const QHostAddress& senderHost, quint16 senderPort);
    void handleAntiEntropyResponse(const Message& message);
    void handleAck(const Message& message);
    void handleRouteRumor(const Message& message, const QHostAddress& senderHost, quint16 senderPort);  // PA3

    void sendDirectMessage(const Message& message, const QString& peerId, bool requireAck = true);
    void sendBroadcastMessage(const Message& message);
    void sendWithRetry(const Message& message, const QString& peerId);
    void sendDatagram(const QByteArray& datagram, const QHostAddress& host, quint16 port);

    void updateVectorClock(const QString& origin, int sequenceNumber);
    void performAntiEntropy();
    void syncMissingMessages(const QString& peerId);

    bool hasMessage(const QString& messageId) const;
    void storeMessage(const Message& message);
    QList<Message> getMissingMessages(const QVariantMap& remoteVectorClock) const;

    QString findPeerIdByAddress(const QHostAddress& host, quint16 port) const;

    // PA3: Routing functions
    void updateRoutingTable(const QString& origin, int seqNo, const QString& nextHop,
                           const QString& nextHopIP, quint16 nextHopPort, bool isDirect);
    bool forwardMessage(Message& message);
    void forwardRumorToRandomNeighbor(const Message& message, const QHostAddress& excludeHost, quint16 excludePort);

    QUdpSocket* socket;
    QString nodeId;
    int serverPort;

    // Peer management
    QMap<QString, PeerInfo> peers;  // peerId -> PeerInfo
    QTimer* antiEntropyTimer;
    QTimer* ackCheckTimer;
    QTimer* peerHealthTimer;
    QTimer* routeRumorTimer;  // PA3: Timer for route rumors

    // Message management
    QMap<QString, Message> messageStore;  // messageId -> Message
    QVariantMap vectorClock;  // origin -> max sequence number seen

    // Reliable delivery
    struct PendingMessage {
        Message message;
        QString targetPeerId;
        qint64 sentTime;
        int retryCount;
    };
    QMap<QString, PendingMessage> pendingAcks;  // messageId -> PendingMessage
    QMap<QString, int> nextSequenceNumbers;  // destination -> next sequence number

    // PA3: Routing table
    QMap<QString, RouteInfo> routingTable;  // destination -> RouteInfo
    int routeSeqNo;  // Our own route sequence number
    bool noForwardMode;  // If true, don't forward chat messages (rendezvous mode)

    // Configuration
    static const int ANTI_ENTROPY_INTERVAL = 2000;  // 2 seconds
    static const int ACK_CHECK_INTERVAL = 1000;  // 1 second
    static const int ACK_TIMEOUT = 2000;  // 2 seconds
    static const int MAX_RETRIES = 3;
    static const int PEER_HEALTH_CHECK_INTERVAL = 5000;  // 5 seconds
    static const int PEER_TIMEOUT = 15000;  // 15 seconds
    static const int ROUTE_RUMOR_INTERVAL = 60000;  // 60 seconds (1 minute)
};
