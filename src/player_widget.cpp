#include "player_widget.h"

#include <utility>
#include "ui_player_widget.h"

player_widget::player_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::player_widget)
{
    ui->setupUi(this);
    Player = new player();
    connect(Player, SIGNAL(sendQImage), this, SLOT(receiveImage));
    connect(Player, &player::finished, Player, &player::deleteLater);
}

player_widget::~player_widget()
{
    if(Player->isRunning())
        stop();
    delete ui;
}

void player_widget::setUrl(QString Url)
{
    Player->setUrl(std::move(Url));
}

void player_widget::play()
{
    stop();
    Player->start();
}

void player_widget::stop()
{
    if(Player->isRunning())
    {
        Player->requestInterruption();
        Player->quit();
        Player->wait(50);
    }
    Image.fill(Qt::black);
}

void player_widget::paintEvent(QPaintEvent *Event)
{
    QPainter painter(this);
    painter.drawImage(0, 0, Image);
}

void player_widget::receiveImage(QImage &Img)
{
    Image = Img.scaled(this->size());
    update();
}
