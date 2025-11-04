#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QMap>

class ChatWindow : public QWidget {
    Q_OBJECT

public:
    explicit ChatWindow(QWidget* parent = nullptr);

    void appendMessage(const QString& message);
    void appendMessageToConversation(const QString& nodeId, const QString& message);
    void appendSentMessage(const QString& nodeId, const QString& message);
    void appendReceivedMessage(const QString& nodeId, const QString& message);
    void setNodeId(const QString& nodeId);
    QString getSelectedDestination() const;
    void updatePeerList(const QList<QString>& peers);
    void updatePeerStatus(const QString& peerId, bool active);

signals:
    void messageEntered(const QString& message, const QString& destination);
    void addPeerRequested(const QString& host, int port);

private slots:
    void onSendClicked();
    void onReturnPressed();
    void onTabChanged(int index);
    void onBroadcastClicked();
    void onAddPeerClicked();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    QTextEdit* getOrCreateConversation(const QString& nodeId);
    void updateInputVisibility();
    QString getCurrentTabDestination() const;

    QTextEdit* systemLog;
    QTabWidget* conversationTabs;
    QTextEdit* messageInput;
    QPushButton* sendButton;
    QPushButton* broadcastButton;
    QLabel* nodeLabel;
    QComboBox* destinationCombo;
    QWidget* inputContainer;
    QLabel* destLabel;
    QLineEdit* peerAddressInput;
    QPushButton* addPeerButton;
    QString currentNodeId;
    QMap<QString, QTextEdit*> conversations;
    QMap<QString, QString> tabToNodeMap;
    QMap<QString, bool> peerStatus;  // peerId -> isActive
};
