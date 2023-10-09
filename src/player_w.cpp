#include "player_w.h"

#include <utility>

player_w::player_w(QWidget *parent) :
    QWidget(parent)
{
    Player = new player();
    QScreen* screen = QGuiApplication::primaryScreen();
    rect = screen->geometry();
    w = rect.width() - 100;
    h = rect.height() - 100;
    flag = false;
    connect(Player, SIGNAL(sendQImage(QImage)), this, SLOT(receiveImage(QImage)));
    connect(Player, &player::finished, Player, &player::deleteLater);
    connect(this, &player_w::stopPlay, [this](){ Player->setFlag(false); });
    connect(this, &player_w::startPlay, [this](){ Player->setFlag(true); });
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
    emit startPlay();
    Player->start();
}

void player_w::stop()
{
    /*if(Player->isRunning())
    {
        Player->requestInterruption();
        Player->quit();
        Player->wait(50);
    }*/
    emit stopPlay();
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
    if(!flag)
    {
        QSize size = Img.size();
        if (size.height() < h || size.width() < w)
            this->setFixedSize(size);
        else
            this->setFixedSize(w, h);
        flag = true;
    }
    update();
}
