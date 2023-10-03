/**
 * @brief decoder，用于解码、格式转换
 * 过程比较耗时，放在独立线程中运行
 * 使用到的api用法可以看t_decoder.h和t_decoder.cpp
 * */
#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QImage>
#include <QDebug>

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

class player: public QThread
{
    Q_OBJECT
private:
    AVFormatContext     *FmtCtx;
    AVCodec             *VideoCodec;
    AVCodecContext      *VideoCodecContext;
    AVCodec             *AudioCodec;
    AVCodecContext      *AudioCodecContext;
    AVPacket            *Pkt;
    AVFrame             *RgbFrame;
    AVFrame             *YuvFrame;
    struct SwsContext   *ImgCtx;
    const uchar         *OutBuffer;
    int32_t             VideoStreamIndex;
    int32_t             AudioStreamIndex;
    int32_t             NumBytes;
    QString             Url;

public:
    player();
    ~player() override;

protected:
    void run() override;
signals:
    void sendQImage(QImage);
public:
    void setUrl(QString url);
    bool openFile();
};

#endif // DECODER_H
