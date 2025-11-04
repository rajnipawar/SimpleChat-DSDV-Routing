#include "networkmanager.h"
#include <QHostAddress>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent), socket(nullptr), serverPort(0), routeSeqNo(1), noForwardMode(false) {

    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::readyRead, this, &NetworkManager::onDataReceived);

    // Anti-entropy timer for periodic synchronization
    antiEntropyTimer = new QTimer(this);
    connect(antiEntropyTimer, &QTimer::timeout, this, &NetworkManager::onAntiEntropyTimeout);

    // ACK check timer for reliable delivery
    ackCheckTimer = new QTimer(this);
    connect(ackCheckTimer, &QTimer::timeout, this, &NetworkManager::checkPendingAcks);

    // Peer health check timer
    peerHealthTimer = new QTimer(this);
    connect(peerHealthTimer, &QTimer::timeout, this, &NetworkManager::checkPeerHealth);

    // PA3: Route rumor timer (60 seconds)
    routeRumorTimer = new QTimer(this);
    connect(routeRumorTimer, &QTimer::timeout, this, &NetworkManager::sendRouteRumor);
}

NetworkManager::~NetworkManager() {
    if (socket) {
        socket->close();
    }
}

bool NetworkManager::startServer(int port) {
    if (!socket->bind(QHostAddress::LocalHost, port)) {
        qDebug() << "Failed to bind UDP socket on port" << port << ":" << socket->errorString();
        return false;
    }

    serverPort = port;
    qDebug() << "UDP server started on port" << port;

    // Start timers
    antiEntropyTimer->start(ANTI_ENTROPY_INTERVAL);
    ackCheckTimer->start(ACK_CHECK_INTERVAL);
    peerHealthTimer->start(PEER_HEALTH_CHECK_INTERVAL);
    routeRumorTimer->start(ROUTE_RUMOR_INTERVAL);

    // PA3: Send initial route rumor on startup
    QTimer::singleShot(1000, this, &NetworkManager::sendRouteRumor);

    return true;
}

void NetworkManager::addPeer(const QString& peerId, const QString& host, int port) {
    if (peerId == nodeId) {
        return;  // Don't add self as peer
    }

    PeerInfo peerInfo(peerId, host, port);
    peers[peerId] = peerInfo;

    // Don't log here, logged in processReceivedMessage
    emit peerDiscovered(peerId, host, port);
}

void NetworkManager::discoverLocalPeers(const QList<int>& portRange) {
    qDebug() << "Discovering peers on local ports:" << portRange;

    // Send discovery message to each port
    for (int port : portRange) {
        if (port == serverPort) {
            continue;  // Skip own port
        }

        Message discoveryMsg("", nodeId, "discovery", 0, Message::ANTI_ENTROPY_REQUEST);
        QByteArray datagram = discoveryMsg.toDatagram();
        sendDatagram(datagram, QHostAddress::LocalHost, port);
    }
}

void NetworkManager::sendMessage(const Message& message) {
    if (!message.isValid() && message.getType() != Message::ANTI_ENTROPY_REQUEST) {
        qDebug() << "Invalid message, not sending";
        return;
    }

    Message msgToSend = message;
    msgToSend.setOrigin(nodeId);

    // Assign sequence number for chat messages
    if (msgToSend.getType() == Message::CHAT_MESSAGE) {
        const QString& destination = msgToSend.getDestination();
        QString seqKey = destination;

        if (!nextSequenceNumbers.contains(seqKey)) {
            nextSequenceNumbers[seqKey] = 1;
        }

        msgToSend.setSequenceNumber(nextSequenceNumbers[seqKey]++);
        msgToSend.setMessageId(msgToSend.generateMessageId());

        // Update own vector clock
        updateVectorClock(nodeId, msgToSend.getSequenceNumber());

        // Store the message
        storeMessage(msgToSend);
    }

    // Set vector clock
    msgToSend.setVectorClock(vectorClock);

    if (!msgToSend.getChatText().isEmpty()) {
        qDebug().noquote() << QString("[SEND] %1 -> %2: \"%3\"")
                               .arg(msgToSend.getOrigin())
                               .arg(msgToSend.getDestination())
                               .arg(msgToSend.getChatText());
    }

    if (msgToSend.isBroadcast()) {
        sendBroadcastMessage(msgToSend);
    } else {
        sendDirectMessage(msgToSend, msgToSend.getDestination());
    }
}

void NetworkManager::sendDirectMessage(const Message& message, const QString& peerId, bool requireAck) {
    if (!peers.contains(peerId)) {
        qDebug() << "Unknown peer:" << peerId;
        return;
    }

    PeerInfo& peer = peers[peerId];
    QByteArray datagram = message.toDatagram();
    sendDatagram(datagram, QHostAddress(peer.host), peer.port);

    // For chat messages, track for ACK (only for direct messages, not broadcasts)
    // Only add if not already tracking to avoid overwriting during retries
    // Skip ACK tracking for anti-entropy synced messages (requireAck = false)
    if (requireAck &&
        message.getType() == Message::CHAT_MESSAGE &&
        !message.isBroadcast() &&
        !pendingAcks.contains(message.getMessageId())) {
        PendingMessage pending;
        pending.message = message;
        pending.targetPeerId = peerId;
        pending.sentTime = QDateTime::currentMSecsSinceEpoch();
        pending.retryCount = 0;

        pendingAcks[message.getMessageId()] = pending;
    }
}

void NetworkManager::sendBroadcastMessage(const Message& message) {
    qDebug() << "Broadcasting message to all peers";

    for (auto it = peers.begin(); it != peers.end(); ++it) {
        const PeerInfo& peer = it.value();
        if (peer.isActive) {
            QByteArray datagram = message.toDatagram();
            sendDatagram(datagram, QHostAddress(peer.host), peer.port);
        }
    }

    // For broadcast chat messages, we don't track ACKs (gossip-style)
}

void NetworkManager::sendDatagram(const QByteArray& datagram, const QHostAddress& host, quint16 port) {
    qint64 sent = socket->writeDatagram(datagram, host, port);
    if (sent == -1) {
        qDebug() << "Failed to send datagram:" << socket->errorString();
    }
}

void NetworkManager::onDataReceived() {
    while (socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;

        qint64 received = socket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);

        if (received > 0) {
            Message message = Message::fromDatagram(datagram);

            if (message.getOrigin() == nodeId) {
                // Ignore messages from self
                continue;
            }

            processReceivedMessage(message, senderHost, senderPort);
        }
    }
}

void NetworkManager::processReceivedMessage(const Message& message, const QHostAddress& senderHost, quint16 senderPort) {
    // Update peer info
    QString senderId = message.getOrigin();
    if (!peers.contains(senderId)) {
        addPeer(senderId, senderHost.toString(), senderPort);
        qDebug().noquote() << QString("[PEER] + Discovered: %1 (%2:%3)")
                               .arg(senderId).arg(senderHost.toString()).arg(senderPort);
    } else {
        peers[senderId].lastSeen = QDateTime::currentMSecsSinceEpoch();
        if (!peers[senderId].isActive) {
            peers[senderId].isActive = true;
            emit peerStatusChanged(senderId, true);
        }
    }

    switch (message.getType()) {
        case Message::CHAT_MESSAGE:
            handleChatMessage(message);
            break;
        case Message::ANTI_ENTROPY_REQUEST:
            handleAntiEntropyRequest(message, senderHost, senderPort);
            break;
        case Message::ANTI_ENTROPY_RESPONSE:
            handleAntiEntropyResponse(message);
            break;
        case Message::ACK:
            handleAck(message);
            break;
        case Message::ROUTE_RUMOR:
            handleRouteRumor(message, senderHost, senderPort);
            break;
    }
}

void NetworkManager::handleChatMessage(const Message& message) {
    // PA3: Check if message is for us
    bool isForUs = message.getDestination() == nodeId || message.isBroadcast();
    bool alreadyHave = hasMessage(message.getMessageId());

    // Store message if we haven't seen it
    if (!alreadyHave) {
        storeMessage(message);
        updateVectorClock(message.getOrigin(), message.getSequenceNumber());
    }

    // PA3: If message is for us, deliver it
    if (isForUs && message.getOrigin() != nodeId) {
        // Skip if in noforward mode and it's a chat message
        if (!noForwardMode || message.getChatText().isEmpty()) {
            if (message.isBroadcast() || !alreadyHave) {
                qDebug().noquote() << QString("[MESSAGE] ✓ Received from %1: \"%2\"")
                                       .arg(message.getOrigin()).arg(message.getChatText());
                emit messageReceived(message);
            }
        }

        // Send ACK if it's directly to us (not broadcast) and is new
        if (!alreadyHave && message.getDestination() == nodeId) {
            Message ack("", nodeId, message.getOrigin(), 0, Message::ACK);
            ack.setMessageId(message.getMessageId());
            sendDirectMessage(ack, message.getOrigin());
        }
    } else if (!isForUs && !message.isBroadcast()) {
        // PA3: Message is not for us, try to forward it
        Message forwardMsg = message;
        forwardMessage(forwardMsg);
    }
}

void NetworkManager::handleAntiEntropyRequest(const Message& message, const QHostAddress& senderHost, quint16 senderPort) {
    QString senderId = message.getOrigin();

    // Compare vector clocks
    QVariantMap remoteVectorClock = message.getVectorClock();
    QList<Message> missingMessages = getMissingMessages(remoteVectorClock);

    // Only log if there are missing messages
    if (!missingMessages.isEmpty()) {
        qDebug() << "Anti-entropy: Sending" << missingMessages.size() << "missing messages to" << senderId;
    }

    // Send response with our vector clock
    Message response("", nodeId, senderId, 0, Message::ANTI_ENTROPY_RESPONSE);
    response.setVectorClock(vectorClock);

    // Include missing messages in the response
    // For simplicity, we send them as separate messages
    sendDatagram(response.toDatagram(), senderHost, senderPort);

    for (const Message& msg : missingMessages) {
        sendDatagram(msg.toDatagram(), senderHost, senderPort);
    }
}

void NetworkManager::handleAntiEntropyResponse(const Message& message) {
    // Update our knowledge of what the peer has
    QVariantMap remoteVectorClock = message.getVectorClock();

    // Send missing messages to the peer
    QList<Message> missingMessages = getMissingMessages(remoteVectorClock);

    // Only log if there are missing messages
    if (!missingMessages.isEmpty()) {
        qDebug() << "Anti-entropy: Sending" << missingMessages.size() << "missing messages to" << message.getOrigin();
    }

    for (const Message& msg : missingMessages) {
        sendDirectMessage(msg, message.getOrigin(), false);  // Don't require ACK for anti-entropy sync
    }
}

void NetworkManager::handleAck(const Message& message) {
    QString messageId = message.getMessageId();

    if (pendingAcks.contains(messageId)) {
        // Silently remove from pending - no need to log every ACK
        pendingAcks.remove(messageId);
    }
}

void NetworkManager::onAntiEntropyTimeout() {
    performAntiEntropy();
}

void NetworkManager::performAntiEntropy() {
    if (peers.isEmpty()) {
        return;
    }

    // Send anti-entropy request to a random active peer
    QList<QString> activePeerIds;
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it.value().isActive) {
            activePeerIds.append(it.key());
        }
    }

    if (activePeerIds.isEmpty()) {
        return;
    }

    int randomIndex = QRandomGenerator::global()->bounded(activePeerIds.size());
    QString randomPeerId = activePeerIds[randomIndex];

    Message request("", nodeId, randomPeerId, 0, Message::ANTI_ENTROPY_REQUEST);
    request.setVectorClock(vectorClock);

    // Silent - don't log routine anti-entropy
    sendDirectMessage(request, randomPeerId);
}

void NetworkManager::checkPendingAcks() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QString> toRetry;

    for (auto it = pendingAcks.begin(); it != pendingAcks.end(); ) {
        PendingMessage& pending = it.value();

        if (now - pending.sentTime > ACK_TIMEOUT) {
            if (pending.retryCount < MAX_RETRIES) {
                qDebug() << "Retry sending message" << pending.message.getMessageId()
                         << "attempt" << (pending.retryCount + 1);
                toRetry.append(it.key());
                ++it;
            } else {
                qDebug() << "Message" << pending.message.getMessageId() << "failed after" << MAX_RETRIES << "retries";
                it = pendingAcks.erase(it);
            }
        } else {
            ++it;
        }
    }

    // Retry messages (check if still pending - ACK may have arrived)
    for (const QString& messageId : toRetry) {
        if (pendingAcks.contains(messageId)) {
            PendingMessage& pending = pendingAcks[messageId];
            pending.retryCount++;
            pending.sentTime = now;
            sendDirectMessage(pending.message, pending.targetPeerId);
        }
    }
}

void NetworkManager::checkPeerHealth() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (auto it = peers.begin(); it != peers.end(); ++it) {
        PeerInfo& peer = it.value();

        if (peer.isActive && (now - peer.lastSeen > PEER_TIMEOUT)) {
            qDebug() << "Peer" << peer.peerId << "timed out";
            peer.isActive = false;
            emit peerStatusChanged(peer.peerId, false);
        }
    }
}

void NetworkManager::updateVectorClock(const QString& origin, int sequenceNumber) {
    int currentMax = vectorClock.value(origin, 0).toInt();
    if (sequenceNumber > currentMax) {
        vectorClock[origin] = sequenceNumber;
    }
}

bool NetworkManager::hasMessage(const QString& messageId) const {
    return messageStore.contains(messageId);
}

void NetworkManager::storeMessage(const Message& message) {
    messageStore[message.getMessageId()] = message;
}

QList<Message> NetworkManager::getMissingMessages(const QVariantMap& remoteVectorClock) const {
    QList<Message> missing;

    // Find messages that the remote peer doesn't have
    for (auto it = messageStore.begin(); it != messageStore.end(); ++it) {
        const Message& msg = it.value();
        QString origin = msg.getOrigin();
        int localSeq = msg.getSequenceNumber();
        int remoteSeq = remoteVectorClock.value(origin, 0).toInt();

        if (localSeq > remoteSeq) {
            missing.append(msg);
        }
    }

    return missing;
}

QList<QString> NetworkManager::getActivePeers() const {
    QList<QString> activePeers;
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        // Return all peers, not just active ones
        // This prevents manually added peers from disappearing
        activePeers.append(it.key());
    }
    return activePeers;
}

QString NetworkManager::findPeerIdByAddress(const QHostAddress& host, quint16 port) const {
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        const PeerInfo& peer = it.value();
        if (peer.host == host.toString() && peer.port == port) {
            return peer.peerId;
        }
    }
    return QString();
}
// ==================== PA3: DSDV Routing Functions ====================

void NetworkManager::sendRouteRumor() {
    if (peers.isEmpty()) {
        return;
    }

    // Increment our sequence number
    routeSeqNo++;

    // Create route rumor message
    Message rumor("", nodeId, "broadcast", routeSeqNo, Message::ROUTE_RUMOR);
    rumor.setVectorClock(vectorClock);

    qDebug().noquote() << QString("[ROUTE RUMOR] Broadcasting: %1 (SeqNo: %2)")
                           .arg(nodeId).arg(routeSeqNo);

    // Send to all peers
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        const PeerInfo& peer = it.value();
        if (peer.isActive) {
            QByteArray datagram = rumor.toDatagram();
            sendDatagram(datagram, QHostAddress(peer.host), peer.port);
        }
    }
}

void NetworkManager::handleRouteRumor(const Message& message, const QHostAddress& senderHost, quint16 senderPort) {
    QString origin = message.getOrigin();
    int seqNo = message.getSequenceNumber();
    QString senderIP = message.getLastIP().isEmpty() ? senderHost.toString() : message.getLastIP();
    quint16 senderPortNum = message.getLastPort() == 0 ? senderPort : message.getLastPort();

    // Find sender's node ID
    QString senderId = findPeerIdByAddress(senderHost, senderPort);
    if (senderId.isEmpty()) {
        senderId = QString("Node%1").arg(senderPort);
    }

    // Only log if different from our node
    if (origin != nodeId) {
        qDebug().noquote() << QString("[ROUTE RUMOR] Received from %1: Route to %2 (SeqNo: %3)")
                               .arg(senderId).arg(origin).arg(seqNo);
    }

    // Determine if this is a direct connection
    bool isDirect = (origin == senderId);

    // Update routing table
    updateRoutingTable(origin, seqNo, senderId, senderIP, senderPortNum, isDirect);

    // Forward to a random neighbor (excluding sender)
    forwardRumorToRandomNeighbor(message, senderHost, senderPort);
}

void NetworkManager::updateRoutingTable(const QString& origin, int seqNo, const QString& nextHop,
                                        const QString& nextHopIP, quint16 nextHopPort, bool isDirect) {
    if (origin == nodeId) {
        return;  // Don't add route to ourselves
    }

    bool shouldUpdate = false;

    if (!routingTable.contains(origin)) {
        // New route
        shouldUpdate = true;
    } else {
        RouteInfo& existingRoute = routingTable[origin];

        // DSDV update logic: prefer higher sequence number, or same sequence with direct route
        if (seqNo > existingRoute.seqNo) {
            shouldUpdate = true;
        } else if (seqNo == existingRoute.seqNo && isDirect && !existingRoute.isDirect) {
            shouldUpdate = true;
        }
    }

    if (shouldUpdate) {
        routingTable[origin] = RouteInfo(nextHop, nextHopIP, nextHopPort, seqNo, isDirect);
        QString routeType = isDirect ? "Direct" : "Via " + nextHop;
        qDebug().noquote() << QString("  [ROUTING TABLE] %1 -> %2 (SeqNo: %3)")
                               .arg(origin, -12).arg(routeType, -20).arg(seqNo);

        // Add the next hop as a peer if not already known
        if (!peers.contains(nextHop)) {
            addPeer(nextHop, nextHopIP, nextHopPort);
        }
    }
}

bool NetworkManager::forwardMessage(Message& message) {
    // Check hop limit
    if (message.getHopLimit() == 0) {
        qDebug().noquote() << "[FORWARD] ✗ Message hop limit reached, dropping";
        return false;
    }

    // Decrement hop limit
    message.setHopLimit(message.getHopLimit() - 1);

    QString dest = message.getDestination();

    // Look up route in routing table
    if (!routingTable.contains(dest)) {
        qDebug().noquote() << QString("[FORWARD] ✗ No route to %1").arg(dest);
        return false;
    }

    const RouteInfo& route = routingTable[dest];

    // Send to next hop
    QByteArray datagram = message.toDatagram();
    sendDatagram(datagram, QHostAddress(route.nextHopIP), route.nextHopPort);

    qDebug().noquote() << QString("[FORWARD] ✓ %1 -> %2 via %3 (HopLimit: %4)")
                           .arg(message.getOrigin()).arg(dest)
                           .arg(route.nextHop).arg(message.getHopLimit());

    return true;
}

void NetworkManager::forwardRumorToRandomNeighbor(const Message& message, const QHostAddress& excludeHost, quint16 excludePort) {
    if (peers.isEmpty()) {
        return;
    }

    // Get list of active peers excluding the sender
    QList<QString> candidates;
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        const PeerInfo& peer = it.value();
        if (peer.isActive && !(peer.host == excludeHost.toString() && peer.port == excludePort)) {
            candidates.append(it.key());
        }
    }

    if (candidates.isEmpty()) {
        return;
    }

    // Pick random neighbor
    int randomIndex = QRandomGenerator::global()->bounded(candidates.size());
    QString randomPeerId = candidates[randomIndex];
    const PeerInfo& randomPeer = peers[randomPeerId];

    // Create new rumor with updated LastIP/LastPort
    Message forwardRumor = message;
    forwardRumor.setLastIP(QHostAddress(QHostAddress::LocalHost).toString());
    forwardRumor.setLastPort(serverPort);

    QByteArray datagram = forwardRumor.toDatagram();
    sendDatagram(datagram, QHostAddress(randomPeer.host), randomPeer.port);

    // Only log forwarding for non-self rumors
    if (message.getOrigin() != nodeId) {
        qDebug().noquote() << QString("  [GOSSIP] Forwarding to %1").arg(randomPeerId);
    }
}
