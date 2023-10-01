#ifndef PLAYER_WIDGET_H
#define PLAYER_WIDGET_H

#include <QWidget>

#include "decoder.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

namespace Ui {
class player_widget;
}

class player_widget : public QWidget
{
    Q_OBJECT

private:
    decoder *decoder;
    QImage image;

public:
    explicit player_widget(QWidget *parent = nullptr);
    ~player_widget();

public:
    void setUrl(QString Url);
    void play();
    void stop();

protected:
    void paintEvent(QPaintEvent *Event);

private slots:
    void receiveImage(QImage &Image);

private:
    Ui::player_widget *ui;
};

#endif // PLAYER_WIDGET_H
