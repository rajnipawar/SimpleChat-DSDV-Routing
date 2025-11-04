#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include "simplechat.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName("SimpleChat P2P - PA3");
    QApplication::setApplicationVersion("3.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("SimpleChat - P2P Messaging with DSDV Routing and NAT Traversal");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Port number for this node (9001-9004)", "port", "9001");
    parser.addOption(portOption);

    QCommandLineOption peersOption(QStringList() << "peers",
                                   "Comma-separated list of peer ports (e.g., 9001,9002,9003,9004)", "peers");
    parser.addOption(peersOption);

    // PA3: Add -noforward option for rendezvous server mode
    QCommandLineOption noforwardOption(QStringList() << "noforward",
                                       "Run in rendezvous server mode (forward route rumors only, not chat messages)");
    parser.addOption(noforwardOption);

    // PA3: Add -connect option to connect to a specific peer
    QCommandLineOption connectOption(QStringList() << "connect",
                                     "Connect to a specific port (e.g., for rendezvous server)", "connect");
    parser.addOption(connectOption);

    parser.process(app);

    bool ok;
    int port = parser.value(portOption).toInt(&ok);

    if (!ok || port < 1024 || port > 65535) {
        qDebug() << "Invalid port number. Using default port 9001.";
        port = 9001;
    }

    // Parse peer ports if provided
    QList<int> peerPorts;
    if (parser.isSet(peersOption)) {
        QString peersStr = parser.value(peersOption);
        QStringList peersList = peersStr.split(',');
        for (const QString& peerStr : peersList) {
            int peerPort = peerStr.trimmed().toInt(&ok);
            if (ok && peerPort >= 1024 && peerPort <= 65535) {
                peerPorts.append(peerPort);
            }
        }
    }

    // PA3: Add connect port if specified
    if (parser.isSet(connectOption)) {
        int connectPort = parser.value(connectOption).toInt(&ok);
        if (ok && connectPort >= 1024 && connectPort <= 65535) {
            peerPorts.append(connectPort);
            qDebug() << "Connecting to rendezvous server on port" << connectPort;
        }
    }

    // PA3: Check noforward mode
    bool noforwardMode = parser.isSet(noforwardOption);
    if (noforwardMode) {
        qDebug() << "Running in NOFORWARD mode (rendezvous server)";
    }

    SimpleChat chat(port, peerPorts, noforwardMode);
    chat.show();

    return app.exec();
}
