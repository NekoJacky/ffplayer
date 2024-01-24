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
#include <QFile>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>

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
#include <libswresample/swresample.h>
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
    AVPacket            *Pkt;
    AVFrame             *RgbFrame;
    AVFrame             *YuvFrame;
    struct SwsContext   *ImgCtx;
    const uchar         *OutBuffer;
    int32_t             VideoStreamIndex;
    int32_t             NumBytes;
    QString             Url;
    bool                StopFlag;

    int32_t             AudioStreamIndex;
    AVCodec             *AudioCodec;
    AVCodecContext      *AudioCodecContext;

public:
    player();
    ~player() override;

protected:
    void run() override;
signals:
    void sendQImage(QImage);
public:
    void setUrl(QString url);
    void setFlag(bool f);
    bool openFile();
};

class AudioPlayer: public QThread
{
    Q_OBJECT
private:
    AVFormatContext     *pAudioFmtCtx;
    AVCodec             *pAudioCodec;
    AVCodecContext      *pAudioCodecCtx;
    AVPacket            *pkt;
    AVFrame             *pAudioFrame;
    SwrContext          *pSwrCtx;
    uint8_t             *Buffer;
    int32_t             OutChannels;
    int32_t             OutSampleRate;
    enum AVSampleFormat OutSampleFmt;
    AVCodecParameters   *pAudioCodecPara;
    int32_t             AudioStreamIndex;
    int32_t             NumBytes;
    QAudioSink          *pAudioSink;
    QIODevice           *pIODevice;
    QString             Url;
    int32_t             ret;
    bool                Flag;

    const int64_t       max_audio_frame_size;

public:
    AudioPlayer(): pAudioFmtCtx(nullptr), pAudioCodec(nullptr),
                   pAudioCodecCtx(nullptr), pkt(av_packet_alloc()),
                   pAudioFrame(av_frame_alloc()), pSwrCtx(nullptr),
                   Buffer(nullptr), OutChannels(-1), OutSampleRate(-1),
                   OutSampleFmt(AV_SAMPLE_FMT_FLTP),
                   pAudioCodecPara(nullptr),
                   AudioStreamIndex(-1), NumBytes(-1),
                   Url(""), ret(-1), max_audio_frame_size(192000),
                   Flag(false), pIODevice(nullptr) {
        QMediaDevices MediaDevices;
        QAudioDevice pAudioDevice = QMediaDevices::defaultAudioOutput();
        QAudioFormat AudioFmt = pAudioDevice.preferredFormat();
        pAudioSink = new QAudioSink(pAudioDevice, AudioFmt);
        pAudioSink->setVolume(15);
    }
    ~AudioPlayer() override;
protected:
    void run() override;
public:
    void setUrl(QString url);
    int32_t openFile();
    void setFlag(bool flag);
};

#endif // DECODER_H
