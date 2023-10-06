#include "widget.h"

#include <utility>
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    // test
    setUrl("");

    connect(ui->PlayBtn, &QPushButton::clicked, this, &Widget::clickedPlayBtn);
    connect(ui->StopBtn, &QPushButton::clicked, this, &Widget::clickedStopBtn);
    connect(ui->OpenFileBtn, &QPushButton::clicked, [this](){
        auto *Dlg = new QFileDialog();
        this->FilePath = QFileDialog::getOpenFileName();
        setUrl(FilePath);
        delete Dlg;
    });
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
