#include "widget.h"

#include <utility>
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    // test
    setUrl(R"(D:\Project\C\ffplayer\test\videos\test_mp4.mp4)");

    connect(ui->PlayBtn, &QPushButton::clicked, this, &Widget::clickedPlayBtn);
    connect(ui->StopBtn, &QPushButton::clicked, this, &Widget::clickedStopBtn);
}

Widget::~Widget()
{
    delete ui;
}


void Widget::setUrl(QString Url_)
{
    ui->player->setUrl(std::move(Url_));
}

void Widget::clickedPlayBtn()
{
    ui->player->play();
}

void Widget::clickedStopBtn()
{
    ui->player->stop();
}
