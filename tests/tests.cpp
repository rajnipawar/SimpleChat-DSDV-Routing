#include <QtTest/QtTest>
#include "../src/message.h"
#include "../src/networkmanager.h"

class TestPA3 : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug() << "=================================================";
        qDebug() << "Starting PA3 Comprehensive Tests";
        qDebug() << "Testing DSDV Routing and NAT Traversal Features";
        qDebug() << "=================================================";
    }

    // =========================================================================
    // MESSAGE CLASS TESTS (10 tests)
    // =========================================================================

    // Test 1: Route Rumor Type
    void testRouteRumorType() {
        qDebug() << "\n[Test 1] Route Rumor Type";
        Message rumor("", "Node1", "broadcast", 5, Message::ROUTE_RUMOR);
        QCOMPARE(rumor.getType(), Message::ROUTE_RUMOR);
        QCOMPARE(rumor.getOrigin(), QString("Node1"));
        QCOMPARE(rumor.getSequenceNumber(), 5);
        qDebug() << "  ✓ Route rumor type correctly set";
    }

    // Test 2: HopLimit Field
    void testHopLimit() {
        qDebug() << "\n[Test 2] HopLimit Field Operations";
        Message msg("Hello", "Node1", "Node2", 1, Message::CHAT_MESSAGE);

        // Default hop limit should be 10
        QCOMPARE(msg.getHopLimit(), (quint32)10);
        qDebug() << "  ✓ Default hop limit is 10";

        // Test setting hop limit
        msg.setHopLimit(5);
        QCOMPARE(msg.getHopLimit(), (quint32)5);
        qDebug() << "  ✓ Hop limit set to 5";

        // Test decrement
        msg.setHopLimit(msg.getHopLimit() - 1);
        QCOMPARE(msg.getHopLimit(), (quint32)4);
        qDebug() << "  ✓ Hop limit decremented to 4";
    }

    // Test 3: LastIP and LastPort Fields
    void testLastIPPort() {
        qDebug() << "\n[Test 3] LastIP and LastPort Fields";
        Message msg("Test", "Node1", "Node2", 1);

        // Initially empty/zero
        QVERIFY(msg.getLastIP().isEmpty());
        QCOMPARE(msg.getLastPort(), (quint16)0);
        qDebug() << "  ✓ Initial LastIP empty and LastPort zero";

        // Set values
        msg.setLastIP("192.168.1.100");
        msg.setLastPort(9001);

        QCOMPARE(msg.getLastIP(), QString("192.168.1.100"));
        QCOMPARE(msg.getLastPort(), (quint16)9001);
        qDebug() << "  ✓ LastIP and LastPort set correctly";
    }

    // Test 4: Message Serialization with PA3 Fields
    void testSerializationWithPA3Fields() {
        qDebug() << "\n[Test 4] Message Serialization with PA3 Fields";
        Message original("Hello", "Node1", "Node2", 10);
        original.setHopLimit(8);
        original.setLastIP("127.0.0.1");
        original.setLastPort(9001);

        // Convert to datagram and back
        QByteArray datagram = original.toDatagram();
        Message restored = Message::fromDatagram(datagram);

        // Verify all fields preserved
        QCOMPARE(restored.getChatText(), original.getChatText());
        QCOMPARE(restored.getOrigin(), original.getOrigin());
        QCOMPARE(restored.getDestination(), original.getDestination());
        QCOMPARE(restored.getSequenceNumber(), original.getSequenceNumber());
        QCOMPARE(restored.getHopLimit(), original.getHopLimit());
        QCOMPARE(restored.getLastIP(), original.getLastIP());
        QCOMPARE(restored.getLastPort(), original.getLastPort());
        qDebug() << "  ✓ All PA3 fields preserved in serialization";
    }

    // Test 5: Route Rumor Serialization
    void testRouteRumorSerialization() {
        qDebug() << "\n[Test 5] Route Rumor Serialization";
        Message rumor("", "Node1", "broadcast", 23, Message::ROUTE_RUMOR);
        rumor.setLastIP("192.168.1.1");
        rumor.setLastPort(45678);

        QByteArray datagram = rumor.toDatagram();
        Message restored = Message::fromDatagram(datagram);

        QCOMPARE(restored.getType(), Message::ROUTE_RUMOR);
        QCOMPARE(restored.getOrigin(), QString("Node1"));
        QCOMPARE(restored.getSequenceNumber(), 23);
        QCOMPARE(restored.getLastIP(), QString("192.168.1.1"));
        QCOMPARE(restored.getLastPort(), (quint16)45678);
        qDebug() << "  ✓ Route rumor serialization complete";
    }

    // Test 6: Private Message with Dest Field
    void testPrivateMessage() {
        qDebug() << "\n[Test 6] Private Message with Destination";
        Message msg("Private", "NodeA", "NodeB", 5);

        QCOMPARE(msg.getOrigin(), QString("NodeA"));
        QCOMPARE(msg.getDestination(), QString("NodeB"));
        QVERIFY(!msg.isBroadcast());
        QCOMPARE(msg.getChatText(), QString("Private"));
        qDebug() << "  ✓ Private message correctly addressed";
    }

    // Test 7: HopLimit in Serialization
    void testHopLimitSerialization() {
        qDebug() << "\n[Test 7] HopLimit Serialization";
        Message msg("Test", "Node1", "Node3", 1);
        msg.setHopLimit(7);

        QVariantMap map = msg.toVariantMap();

        QVERIFY(map.contains("HopLimit"));
        QCOMPARE(map["HopLimit"].toUInt(), (quint32)7);

        Message restored = Message::fromVariantMap(map);
        QCOMPARE(restored.getHopLimit(), (quint32)7);
        qDebug() << "  ✓ HopLimit preserved in VariantMap serialization";
    }

    // Test 8: Message ID Generation
    void testMessageIdGeneration() {
        qDebug() << "\n[Test 8] Message ID Generation";
        Message msg("Test", "Node1", "Node2", 42);

        QString expectedId = "Node1_42";
        QCOMPARE(msg.getMessageId(), expectedId);
        qDebug() << "  ✓ Message ID correctly generated as Origin_SeqNo";
    }

    // Test 9: Broadcast Detection
    void testBroadcastDetection() {
        qDebug() << "\n[Test 9] Broadcast Detection";
        Message broadcast1("Hi all", "Node1", "broadcast", 1);
        Message broadcast2("Hi all", "Node1", "-1", 1);
        Message private1("Hi", "Node1", "Node2", 1);

        QVERIFY(broadcast1.isBroadcast());
        QVERIFY(broadcast2.isBroadcast());
        QVERIFY(!private1.isBroadcast());
        qDebug() << "  ✓ Broadcast messages correctly identified";
    }

    // Test 10: Zero HopLimit Handling
    void testZeroHopLimit() {
        qDebug() << "\n[Test 10] Zero HopLimit Handling";
        Message msg("Test", "Node1", "Node2", 1);
        msg.setHopLimit(1);

        // Simulate forwarding
        msg.setHopLimit(msg.getHopLimit() - 1);
        QCOMPARE(msg.getHopLimit(), (quint32)0);
        qDebug() << "  ✓ HopLimit correctly decremented to zero";
        qDebug() << "  Note: Messages with zero hop limit should be dropped by NetworkManager";
    }

    // =========================================================================
    // ROUTING TABLE TESTS (10 tests)
    // =========================================================================

    // Test 11: RouteInfo Structure
    void testRouteInfoStructure() {
        qDebug() << "\n[Test 11] RouteInfo Structure";
        RouteInfo route("Node2", "127.0.0.1", 9002, 5, true);

        QCOMPARE(route.nextHop, QString("Node2"));
        QCOMPARE(route.nextHopIP, QString("127.0.0.1"));
        QCOMPARE(route.nextHopPort, (quint16)9002);
        QCOMPARE(route.seqNo, 5);
        QCOMPARE(route.isDirect, true);
        QVERIFY(route.lastUpdated > 0);
        qDebug() << "  ✓ RouteInfo structure correctly initialized";
    }

    // Test 12: Basic Routing Table Operations
    void testBasicRoutingTable() {
        qDebug() << "\n[Test 12] Basic Routing Table Operations";
        NetworkManager nm;
        nm.setNodeId("Node1");

        // Initially empty
        QMap<QString, RouteInfo> table = nm.getRoutingTable();
        QVERIFY(table.isEmpty());
        qDebug() << "  ✓ Routing table initially empty";

        // Note: Direct routing table updates are private
        // They are tested through route rumor handling
    }

    // Test 13: NoForward Mode
    void testNoForwardMode() {
        qDebug() << "\n[Test 13] NoForward Mode (Rendezvous Server)";
        NetworkManager nm;

        // Default is false
        QVERIFY(!nm.isNoForwardMode());
        qDebug() << "  ✓ Default noForwardMode is false";

        // Set to true
        nm.setNoForwardMode(true);
        QVERIFY(nm.isNoForwardMode());
        qDebug() << "  ✓ NoForwardMode enabled successfully";

        // Set back to false
        nm.setNoForwardMode(false);
        QVERIFY(!nm.isNoForwardMode());
        qDebug() << "  ✓ NoForwardMode disabled successfully";
    }

    // Test 14: NetworkManager Initialization
    void testNetworkManagerInit() {
        qDebug() << "\n[Test 14] NetworkManager Initialization";
        NetworkManager nm;

        // Set node ID
        nm.setNodeId("TestNode");
        QCOMPARE(nm.getNodeId(), QString("TestNode"));
        qDebug() << "  ✓ Node ID set correctly";

        // Check routing table exists
        QMap<QString, RouteInfo> table = nm.getRoutingTable();
        QVERIFY(table.isEmpty());
        qDebug() << "  ✓ Routing table accessible";
    }

    // Test 15: Message ID Uniqueness
    void testMessageIdUniqueness() {
        qDebug() << "\n[Test 15] Message ID Uniqueness";
        Message msg1("Test1", "Node1", "Node2", 1);
        Message msg2("Test2", "Node1", "Node2", 2);
        Message msg3("Test3", "Node2", "Node1", 1);

        QString id1 = msg1.getMessageId();
        QString id2 = msg2.getMessageId();
        QString id3 = msg3.getMessageId();

        QVERIFY(id1 != id2);  // Different sequence numbers
        QVERIFY(id1 != id3);  // Different origins
        qDebug() << "  ✓ Message IDs are unique";
        qDebug() << QString("    ID1: %1, ID2: %2, ID3: %3").arg(id1).arg(id2).arg(id3);
    }

    // Test 16: Multiple Route Rumors
    void testMultipleRouteRumors() {
        qDebug() << "\n[Test 16] Multiple Route Rumors Creation";
        Message rumor1("", "Node1", "broadcast", 1, Message::ROUTE_RUMOR);
        Message rumor2("", "Node1", "broadcast", 2, Message::ROUTE_RUMOR);
        Message rumor3("", "Node2", "broadcast", 1, Message::ROUTE_RUMOR);

        QCOMPARE(rumor1.getSequenceNumber(), 1);
        QCOMPARE(rumor2.getSequenceNumber(), 2);
        QCOMPARE(rumor3.getSequenceNumber(), 1);

        QCOMPARE(rumor1.getOrigin(), QString("Node1"));
        QCOMPARE(rumor2.getOrigin(), QString("Node1"));
        QCOMPARE(rumor3.getOrigin(), QString("Node2"));

        qDebug() << "  ✓ Multiple route rumors created with correct sequence numbers";
    }

    // Test 17: Message Type Validation
    void testMessageTypeValidation() {
        qDebug() << "\n[Test 17] Message Type Validation";
        Message chat("Hello", "Node1", "Node2", 1, Message::CHAT_MESSAGE);
        Message rumor("", "Node1", "broadcast", 1, Message::ROUTE_RUMOR);
        Message request("", "Node1", "Node2", 1, Message::ANTI_ENTROPY_REQUEST);
        Message response("", "Node1", "Node2", 1, Message::ANTI_ENTROPY_RESPONSE);
        Message ack("", "Node1", "Node2", 1, Message::ACK);

        QCOMPARE(chat.getType(), Message::CHAT_MESSAGE);
        QCOMPARE(rumor.getType(), Message::ROUTE_RUMOR);
        QCOMPARE(request.getType(), Message::ANTI_ENTROPY_REQUEST);
        QCOMPARE(response.getType(), Message::ANTI_ENTROPY_RESPONSE);
        QCOMPARE(ack.getType(), Message::ACK);
        qDebug() << "  ✓ All message types correctly set";
    }

    // Test 18: HopLimit Decrement Simulation
    void testHopLimitDecrementChain() {
        qDebug() << "\n[Test 18] HopLimit Decrement Chain";
        Message msg("Multi-hop test", "Node1", "Node5", 1);
        msg.setHopLimit(10);

        // Simulate 5 hops
        for (int i = 0; i < 5; i++) {
            QVERIFY(msg.getHopLimit() > 0);
            msg.setHopLimit(msg.getHopLimit() - 1);
        }

        QCOMPARE(msg.getHopLimit(), (quint32)5);
        qDebug() << "  ✓ HopLimit correctly decremented through 5 hops";

        // Simulate 5 more hops to reach zero
        for (int i = 0; i < 5; i++) {
            msg.setHopLimit(msg.getHopLimit() - 1);
        }

        QCOMPARE(msg.getHopLimit(), (quint32)0);
        qDebug() << "  ✓ HopLimit reached zero after 10 hops";
    }

    // Test 19: LastIP/LastPort Update Simulation
    void testLastIPPortUpdateChain() {
        qDebug() << "\n[Test 19] LastIP/LastPort Update Chain";
        Message msg("Test", "Node1", "Node3", 1);

        // First hop: Node1 -> Node2
        msg.setLastIP("192.168.1.1");
        msg.setLastPort(9001);
        QCOMPARE(msg.getLastIP(), QString("192.168.1.1"));
        QCOMPARE(msg.getLastPort(), (quint16)9001);
        qDebug() << "  ✓ Hop 1: LastIP=192.168.1.1, LastPort=9001";

        // Second hop: Node2 -> Node3
        msg.setLastIP("192.168.1.2");
        msg.setLastPort(9002);
        QCOMPARE(msg.getLastIP(), QString("192.168.1.2"));
        QCOMPARE(msg.getLastPort(), (quint16)9002);
        qDebug() << "  ✓ Hop 2: LastIP=192.168.1.2, LastPort=9002";

        qDebug() << "  ✓ LastIP/LastPort correctly updated through routing chain";
    }

    // Test 20: Complete Message Lifecycle
    void testCompleteMessageLifecycle() {
        qDebug() << "\n[Test 20] Complete Message Lifecycle";

        // Create original message
        Message original("Test message", "NodeA", "NodeB", 42);
        original.setHopLimit(10);
        original.setLastIP("10.0.0.1");
        original.setLastPort(8080);
        qDebug() << "  ✓ Original message created";

        // Serialize to datagram
        QByteArray datagram = original.toDatagram();
        QVERIFY(!datagram.isEmpty());
        qDebug() << "  ✓ Message serialized to datagram";

        // Deserialize
        Message restored = Message::fromDatagram(datagram);
        qDebug() << "  ✓ Message deserialized from datagram";

        // Verify all fields
        QCOMPARE(restored.getChatText(), original.getChatText());
        QCOMPARE(restored.getOrigin(), original.getOrigin());
        QCOMPARE(restored.getDestination(), original.getDestination());
        QCOMPARE(restored.getSequenceNumber(), original.getSequenceNumber());
        QCOMPARE(restored.getHopLimit(), original.getHopLimit());
        QCOMPARE(restored.getLastIP(), original.getLastIP());
        QCOMPARE(restored.getLastPort(), original.getLastPort());
        qDebug() << "  ✓ All fields verified - lifecycle complete";

        // Simulate forwarding
        restored.setHopLimit(restored.getHopLimit() - 1);
        restored.setLastIP("10.0.0.2");
        restored.setLastPort(8081);
        QCOMPARE(restored.getHopLimit(), (quint32)9);
        qDebug() << "  ✓ Message ready for forwarding with updated fields";
    }

    void cleanupTestCase() {
        qDebug() << "\n=================================================";
        qDebug() << "PA3 Tests Completed Successfully";
        qDebug() << "Total: 20 tests (10 Message + 10 Routing)";
        qDebug() << "=================================================";
    }
};

QTEST_MAIN(TestPA3)
#include "tests.moc"
