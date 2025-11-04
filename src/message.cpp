#include "message.h"
#include <QJsonDocument>
#include <QJsonObject>

Message::Message() : sequenceNumber(0), type(CHAT_MESSAGE), hopLimit(10), lastPort(0) {}

Message::Message(const QString& chatText, const QString& origin, const QString& destination, int sequenceNumber, MessageType type)
    : chatText(chatText), origin(origin), destination(destination), sequenceNumber(sequenceNumber), type(type), hopLimit(10), lastPort(0) {
    messageId = generateMessageId();
}

Message Message::fromVariantMap(const QVariantMap& map) {
    Message msg;
    msg.chatText = map.value("ChatText").toString();
    msg.origin = map.value("Origin").toString();
    msg.destination = map.value("Destination").toString();
    msg.sequenceNumber = map.value("SequenceNumber").toInt();
    msg.type = static_cast<MessageType>(map.value("Type", CHAT_MESSAGE).toInt());
    msg.vectorClock = map.value("VectorClock").toMap();
    msg.messageId = map.value("MessageId").toString();
    msg.hopLimit = map.value("HopLimit", 10).toUInt();
    msg.lastIP = map.value("LastIP").toString();
    msg.lastPort = map.value("LastPort", 0).toUInt();

    // Generate message ID if not present
    if (msg.messageId.isEmpty()) {
        msg.messageId = msg.generateMessageId();
    }

    return msg;
}

Message Message::fromDatagram(const QByteArray& datagram) {
    QJsonDocument doc = QJsonDocument::fromJson(datagram);
    if (doc.isNull() || !doc.isObject()) {
        return Message();
    }

    QVariantMap map = doc.object().toVariantMap();
    return fromVariantMap(map);
}

QVariantMap Message::toVariantMap() const {
    QVariantMap map;
    map["ChatText"] = chatText;
    map["Origin"] = origin;
    map["Destination"] = destination;
    map["SequenceNumber"] = sequenceNumber;
    map["Type"] = static_cast<int>(type);
    map["VectorClock"] = vectorClock;
    map["MessageId"] = messageId;

    // Add new PA3 fields
    if (hopLimit > 0) {
        map["HopLimit"] = hopLimit;
    }
    if (!lastIP.isEmpty()) {
        map["LastIP"] = lastIP;
    }
    if (lastPort > 0) {
        map["LastPort"] = lastPort;
    }

    return map;
}

QByteArray Message::toDatagram() const {
    QVariantMap map = toVariantMap();
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    return doc.toJson(QJsonDocument::Compact);
}

bool Message::isValid() const {
    return !origin.isEmpty() && !destination.isEmpty() && sequenceNumber >= 1;
}

QString Message::generateMessageId() const {
    return QString("%1_%2").arg(origin).arg(sequenceNumber);
}

QDataStream& operator<<(QDataStream& stream, const Message& message) {
    QVariantMap map = message.toVariantMap();
    stream << map;
    return stream;
}

QDataStream& operator>>(QDataStream& stream, Message& message) {
    QVariantMap map;
    stream >> map;
    message = Message::fromVariantMap(map);
    return stream;
}
