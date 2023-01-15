#include <QCoreApplication>
#include <QFile>

#include <iostream> // std::cout
#include <fstream>

#include "socket/websocketserver.h"
#include "Pong/game.h"

#include <QTimer>
#include <QThread>

// Routes
#include "routes/jsontestroute.h"
#include "routes/httptestroute.h"
#include "routes/requestqueueroute.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Configure qDebug
    qputenv("QT_MESSAGE_PATTERN", "%{file}:%{line} %{message}");

    // Create WebSocket Server instance
    WebSocketServer& wss = WebSocketServer::getInstance(1214);

    Game& game = Game::getInstance();
    game.start();

    return app.exec();
}
