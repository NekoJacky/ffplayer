/**
 * @brief decoder，用于解码、格式转换
 * 过程比较耗时，放在独立线程中运行
 * */
#ifndef DECODER_H
#define DECODER_H

#include <QThread>
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

class decoder: public QThread
{
private:
    bool                file_is_open;
    AVFormatContext     *fmt_cxt;
    AVCodec             *VideoCodec;
    AVCodecContext      *VideoCodecContext;
    AVPacket            *pkt;
    AVFrame             *RgbFrame;
    AVFrame             *YuvFrame;
    struct SwsContext   *img_cxt;
    char                *out_buffer;
    int32_t             VideoStreamIndex;
    int32_t             NumBytes;
    QString             Url;

public:
    decoder();
    ~decoder() override;

protected:
    void run() override;
signals:
    void sendImage(QImage);
public:
    void setUrl(QString url);
    bool openFile();
};

#endif // DECODER_H
