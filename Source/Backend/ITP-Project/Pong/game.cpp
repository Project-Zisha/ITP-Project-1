#include "game.h"

Game& Game::getInstance() {
    static Game instance;
    return instance;
}


Game::Game()
    : m_score1(0),
      m_score2(0),
      m_playingFieldWidth(680),
      m_playingFieldHeight(540),
      m_paddle1(m_paddleLeftX, m_paddleY, 6, 80, ""),
      m_paddle2(m_paddleRightX, m_paddleY, 6, 80, ""),
      m_ball(-10, -10, 19)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(10); // (100 TPS)
    connect(m_timer, &QTimer::timeout, this, &Game::update);

    // Logs
    logs = &LogUtils::getInstance();
    QStringList data = {"paddle1_x", "paddle1_y", "paddle2_x", "paddle2_y", "ball_x", "ball_y", "ball_vx", "ball_vy", "score1", "points1", "score2", "points2", "timestamp"};
    logs->appendData(data);

    // set m_ball position to center of the playing field with radius of ball as offset
    m_ball.setPosition(m_playingFieldWidth / 2 - (m_ball.getDiameter() / 2), m_playingFieldHeight / 2 - (m_ball.getDiameter()) / 2);

    m_winnerBroadcast["code"] = 999;
}

void Game::update()
{
    if (m_ball.getPosition().x() < 0) {
        m_score2++;
        qDebug() << "Player 2 scored. Score: " << m_score2;
        // reset game
        this->reset();
    }

    // If ball hits wall on the right, increase score of player 1
    if (m_ball.getPosition().x() > m_playingFieldWidth) {
        m_score1++;
        qDebug() << "Player 1 scored. Score: " << m_score1;
        this->reset();
    }

    // If one Player reaches 5 points, stop the game and broadcast the winner
    if (m_score1 == m_winningScore) {
        qDebug() << "Player 1 won!";
        m_winnerBroadcast["winner"] = 1;
    } else if (m_score2 == m_winningScore) {
        qDebug() << "Player 2 won!";
        m_winnerBroadcast["winner"] = 2;
    }

    if (m_score1 == m_winningScore || m_score2 == m_winningScore) {
        QJsonDocument doc(m_winnerBroadcast);
        QByteArray jsonData = doc.toJson();
        WebSocketServer::getInstance().broadcast(jsonData);
        
        this->stop();
        return;
    }

    // Check if ball is out of bounds
    m_ball.checkOutOfBounds(m_playingFieldHeight);

    // Adapt this to give extra points for hitting the ball, hitting the paddle edge, ...
    m_paddle1.addScore(1);
    m_paddle2.addScore(1);

    m_ball.checkCollision(&m_paddle1);
    m_ball.checkCollision(&m_paddle2);
    m_ball.updatePosition();
    this->sendState();
}

void Game::start(QQueue<QString> p_queue)
{
    m_queue = p_queue;
    m_paddle1.setId(m_queue[0]);
    m_paddle2.setId(m_queue[1]);

    // Start the game
    m_timer->start();
}


#include <QDebug>
void Game::stop()
{
    // Stop the game
    m_timer->stop();

    // Save the logs to a csv file
    logs->saveToCSV(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".csv");

    // Save Score
    Score::addScore(m_paddle1.getId(), m_paddle1.getScore());
    Score::addScore(m_paddle2.getId(), m_paddle2.getScore());

    // Reset the Paddle Score
    m_paddle1.resetScore();
    m_paddle2.resetScore();

    // Reset the scores
    m_score1 = 0;
    m_score2 = 0;
}

void Game::reset()
{
    // Reset the game
    // you can reset the paddles and ball position, and scores
    m_paddle1.setPosition(m_paddleLeftX, m_paddleY);
    m_paddle2.setPosition(m_paddleRightX, m_paddleY);
    m_ball.setPosition(m_playingFieldWidth / 2 - (m_ball.getDiameter() / 2), m_playingFieldHeight / 2 - (m_ball.getDiameter()) / 2);

    m_ball.setRandomVelocity();
}

void Game::sendState()
{
    // Serialize the state of the game to json
    QJsonObject json;
    json["code"] = 204;
    json["paddle1"] = QJsonObject({
                            {"x", m_paddle1.getPosition().x()},
                            {"y", m_paddle1.getPosition().y()},
                            {"width", m_paddle1.getWidth()},
                            {"height", m_paddle1.getHeight()}
                        });
    json["paddle2"] = QJsonObject({
                            {"x", m_paddle2.getPosition().x()},
                            {"y", m_paddle2.getPosition().y()},
                            {"width", m_paddle2.getWidth()},
                            {"height", m_paddle2.getHeight()}
                        });
    json["ball"] = QJsonObject({
                            {"x", m_ball.getPosition().x()},
                            {"y", m_ball.getPosition().y()},
                            {"vx", m_ball.getVelocity().x()},
                            {"vy", m_ball.getVelocity().y()},
                            {"diameter", m_ball.getDiameter()}
                        });
    json["score1"] = m_score1;
    json["points1"] = m_paddle1.getScore();
    json["score2"] = m_score2;
    json["points2"] = m_paddle2.getScore();
    json["running"] = m_running;

    // Send the json object to all connected clients
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson();

    // Append all data to the log file with a timestamp
    QStringList data = {
    QString::number(m_paddle1.getPosition().x()), // paddle1_x
    QString::number(m_paddle1.getPosition().y()), // paddle1_y
    QString::number(m_paddle2.getPosition().x()), // paddle2_x
    QString::number(m_paddle2.getPosition().y()), // paddle2_y
    QString::number(m_ball.getPosition().x()), // ball_x
    QString::number(m_ball.getPosition().y()), // ball_y
    QString::number(m_ball.getVelocity().x()), // ball_vx
    QString::number(m_ball.getVelocity().y()), // ball_vy
    QString::number(m_score1), // score1
    QString::number(m_paddle1.getScore()), // points1
    QString::number(m_score2), // score2
    QString::number(m_paddle2.getScore()), // points2
    QString::number(QDateTime::currentMSecsSinceEpoch()) // timestamp
    };

    logs->appendData(data);

    // Broadcast jsonData
    WebSocketServer::getInstance().broadcast(jsonData);
}


int Game::getScore(int player)
{
    if (player == 1)
        return m_score1;
    if (player == 2)
        return m_score2;
    return -1;
}

void Game::movePaddle1Up() {
    m_paddle1.moveUp();
}
void Game::movePaddle1Down() {
    m_paddle1.moveDown();
}
void Game::movePaddle2Up() {
    m_paddle2.moveUp();
}
void Game::movePaddle2Down() {
    m_paddle2.moveDown();
}

void Game::setScore(int player, int score)
{
    if (player == 1)
        m_score1 = score;
    if (player == 2)
        m_score2 = score;
}
