#include "player_widget.h"
#include "ui_player_widget.h"

player_widget::player_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::player_widget)
{
    ui->setupUi(this);
}

player_widget::~player_widget()
{
    delete ui;
}
