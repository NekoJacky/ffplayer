#ifndef PLAYER_WIDGET_H
#define PLAYER_WIDGET_H

#include <QWidget>
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>

#include "player.h"

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

class player_w : public QWidget
{
    Q_OBJECT

private:
    player  *Player;
    QImage  Image;
    QRect   rect;
    int32_t h;
    int32_t w;
    bool    flag;

public:
    explicit player_w(QWidget *parent = nullptr);
    ~player_w() override;

public:
    void setUrl(QString Url);
    void play();
    void stop();

protected:
    void paintEvent(QPaintEvent *Event) override;

signals:
    void stopPlay();
    void startPlay();

private slots:
    void receiveImage(const QImage &Img);
};

#endif // PLAYER_WIDGET_H
