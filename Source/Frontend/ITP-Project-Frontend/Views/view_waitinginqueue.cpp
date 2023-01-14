#include "view_waitinginqueue.h"
#include "ui_view_waitinginqueue.h"

view_waitingInQueue* view_waitingInQueue::m_instance = nullptr;

view_waitingInQueue* view_waitingInQueue::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new view_waitingInQueue;
    }
    return m_instance;
}

view_waitingInQueue::view_waitingInQueue(QWidget *parent) : QDialog(parent), ui(new Ui::view_waitingInQueue)
{
    ui->setupUi(this);
    // Connect to Websocket
    wSocketController::getInstance()->connectToServer();
}

void view_waitingInQueue::startGame() {
    qDebug() << "Fucked meta";
}

view_waitingInQueue::~view_waitingInQueue()
{
    delete ui;
}
