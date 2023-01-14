#include "websocketserver.h"

QList<QWebSocket *> WebSocketServer::m_sockets;

WebSocketServer::WebSocketServer(quint16 port, QObject *parent) :
    QObject(parent),
    m_socketServer(QStringLiteral("ITP-Project Server"), QWebSocketServer::NonSecureMode, this)
{
    if (!m_socketServer.listen(QHostAddress::LocalHost, port)) {
        qFatal("Failed to open web socket server.");
    }

    pmm = new playerMovementManager();

    qDebug() << "WebSocket server listening on port" << m_socketServer.serverPort();
    connect(&m_socketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

void WebSocketServer::onNewConnection() {
    QWebSocket *socket = m_socketServer.nextPendingConnection();

    // Give every Connection an ID and send that ID to Player
    // Player will save that ID, it will act as an authentication Token
    QString playerId = QUuid::createUuid().toString();
    try {
        QueueManager::addPlayer(playerId);
    } catch (QueueFullException &e) {
        qDebug() << e.what();
    }
    std::map<std::string, JSONUtils::Value> data{
        {"code", 200},
        {"UUID", playerId.toStdString()},
        {"PlayerNumber", QueueManager::getQueueSize()},
        {"message", "Connection established to SocketServer!"}
    };
    socket->sendTextMessage(QString::fromStdString(JSONUtils::generateJSON(data)));

    // Connect to the socket's signals
    connect(socket, &QWebSocket::textMessageReceived, pmm, &playerMovementManager::onTextMessageReceived);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::onBinaryMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);

    // If queue is full
    if (QueueManager::getQueueSize() == 2) {
        // Send the GameID to the Players
        std::map<std::string, JSONUtils::Value> data{
            {"code", 201},
            {"message", "GameID sent to Players!"},
            {"gameId", QueueManager::getGameId().toString().toStdString()}
        };
        for (QWebSocket *socket : m_sockets) {
            socket->sendTextMessage(QString::fromStdString(JSONUtils::generateJSON(data)));
        }
    }

    m_sockets.append(socket);
}

void WebSocketServer::onTextMessageReceived(QString message) {
    // This method has been deprecated.
}

void WebSocketServer::onBinaryMessageReceived(QByteArray message) {
    qDebug() << "Received binary message of size" << message.size() << "bytes";

    // Send a response back to the client
    QWebSocket *senderSocket = qobject_cast<QWebSocket *>(sender());
    if (senderSocket) {
        std::map<std::string, JSONUtils::Value> data{
          {"code", 202},
          {"message", "Received Binary Message!"},
          {"received_message", message.toStdString()}
        };
        senderSocket->sendBinaryMessage(QString::fromStdString(JSONUtils::generateJSON(data)).toUtf8());
    }
}

void WebSocketServer::onSocketDisconnected() {
    // Get the disconnected socket
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());

    qDebug() << "Disconnected one User.";

    // Remove the socket from the list of connected sockets
    m_sockets.removeOne(socket);

    // Notify Players
    std::map<std::string, JSONUtils::Value> data{
        {"code", 203},
        {"message", "One Player disconnected!"}
    };
    for (QWebSocket *socket : m_sockets) {
        socket->sendTextMessage(QString::fromStdString(JSONUtils::generateJSON(data)));
    }

    // Broadcast message
    this->broadcast("One Player disconnected!");

    // Disconnected all Users

    // Reset the Queue
    QueueManager::resetQueue();

    // Delete the socket
    socket->deleteLater();
}

void WebSocketServer::broadcast(const QString &message) {
    // Iterate over the list of connected sockets
    for (auto socket : m_sockets) {
        // Send the message to the socket
        socket->sendTextMessage(message);
    }
}
