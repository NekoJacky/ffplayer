#include "player_w.h"

#include <utility>

player_w::player_w(QWidget *parent) :
    QWidget(parent)
{
    Player = new player();
    connect(Player, SIGNAL(sendQImage(QImage)), this, SLOT(receiveImage(QImage)));
    connect(Player, &player::finished, Player, &player::deleteLater);
}

player_w::~player_w()
{
    if(Player->isRunning())
        stop();
}

void player_w::setUrl(QString Url)
{
    Player->setUrl(std::move(Url));
}

void player_w::play()
{
    stop();
    Player->start();
}

void player_w::stop()
{
    if(Player->isRunning())
    {
        Player->requestInterruption();
        Player->quit();
        Player->wait(50);
    }
    Image.fill(Qt::black);
}

void player_w::paintEvent(QPaintEvent *Event)
{
    QPainter painter(this);
    painter.drawImage(0, 0, Image);
}

void player_w::receiveImage(const QImage &Img)
{
    Image = Img.scaled(this->size());
    update();
}
