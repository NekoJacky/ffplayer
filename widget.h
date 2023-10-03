#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

public:
    void setUrl(QString Url_);

private slots:
    void clickedPlayBtn();
    void clickedStopBtn();

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
