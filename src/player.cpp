#include "player.h"

#include <utility>

player::player():
    FmtCtx(avformat_alloc_context()),
    VideoCodec(nullptr),
    VideoCodecContext(nullptr),
    AudioCodec(nullptr),
    AudioCodecContext(nullptr),
    Pkt(av_packet_alloc()),
    RgbFrame(av_frame_alloc()),
    YuvFrame(av_frame_alloc()),
    ImgCtx(nullptr),
    OutBuffer(nullptr),
    VideoStreamIndex(-1),
    AudioStreamIndex(-1),
    NumBytes(-1),
    Url(""),
    StopFlag(true)
{}

player::~player()
{
    if(!FmtCtx) avformat_close_input(&FmtCtx);
    if(!VideoCodecContext) avcodec_close(VideoCodecContext);
    if(!VideoCodecContext) avcodec_free_context(&VideoCodecContext);
    if(!Pkt) av_packet_free(&Pkt);
    if(!RgbFrame) av_frame_free(&RgbFrame);
    if(!YuvFrame) av_frame_free(&YuvFrame);
    if(!ImgCtx) sws_freeContext(ImgCtx);
}

void player::setUrl(QString url)
{
    Url = std::move(url);
}

void player::setFlag(bool f)
{
    StopFlag = f;
}

bool player::openFile()
{
    if(Url.isEmpty())
    {
        qDebug() << "<Open> Empty File Path";
        return false;
    }

    if(avformat_open_input(&FmtCtx, Url.toLocal8Bit().data(), nullptr, nullptr) < 0)
    {
        qDebug() << "<Open> Can't open file";
        return false;
    }

    if(avformat_find_stream_info(FmtCtx, nullptr) < 0)
    {
        qDebug() << "<Open> Can't find stream";
        return false;
    }

    auto StreamCnt = (int32_t)(FmtCtx->nb_streams);
    for(int i = 0; i < StreamCnt; i++)
    {
        if (FmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if(FmtCtx->streams[i]->codecpar->width <= 0 || FmtCtx->streams[i]->codecpar->height <= 0)
            {
                qDebug() << "Invalid video stream, stream index:" << i;
                continue;
            }
            VideoStreamIndex = i;
            continue;
        }
        if(FmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if(FmtCtx->streams[i]->codecpar->ch_layout.nb_channels <= 0 ||
               FmtCtx->streams[i]->codecpar->sample_rate <= 0)
            {
                qDebug() << "Invalid audio stream, stream index:" << i;
                continue;
            }
            AudioStreamIndex = i;
            continue;
        }
    }

    AVCodecParameters *VideoPara = FmtCtx->streams[VideoStreamIndex]->codecpar;
    AVCodecParameters *AudioPara = FmtCtx->streams[AudioStreamIndex]->codecpar;

    if(!(VideoCodec = const_cast<AVCodec*>(avcodec_find_decoder(VideoPara->codec_id))))
    {
        qDebug() << "<Open> Can't find video decoder codec";
        return false;
    }
    if(!(VideoCodecContext = avcodec_alloc_context3(VideoCodec)))
    {
        qDebug() << "<Open> Can't find video decoder codec context";
        return false;
    }
    if(!(AudioCodec = const_cast<AVCodec*>(avcodec_find_decoder(AudioPara->codec_id))))
    {
        qDebug() << "<Open> Can't find audio decoder codec";
        return false;
    }
    if(!(AudioCodecContext = avcodec_alloc_context3(AudioCodec)))
    {
        qDebug() << "<Open> Can't find audio decoder codec context";
        return false;
    }

    if(avcodec_parameters_to_context(VideoCodecContext, VideoPara) < 0)
    {
        qDebug() << "<Open> Can't video initialize parameter";
        return false;
    }
    if(avcodec_parameters_to_context(AudioCodecContext, AudioPara) < 0)
    {
        qDebug() << "<Open> Can't initialize audio parameter";
        return false;
    }
    if(avcodec_open2(VideoCodecContext, VideoCodec, nullptr) < 0)
    {
        qDebug() << "<Open> Can't open_yuv video codec";
        return false;
    }
    if(avcodec_open2(AudioCodecContext, AudioCodec, nullptr) < 0)
    {
        qDebug() << "<Open> Can't open_yuv audio codec";
        return false;
    }

    // 创建格式转换器，指定缩放算法，不添加滤镜
    ImgCtx = sws_getContext(VideoCodecContext->width,
                            VideoCodecContext->height,
                            VideoCodecContext->pix_fmt,
                            VideoCodecContext->width,
                            VideoCodecContext->height,
                            AV_PIX_FMT_RGB32, SWS_BICUBIC,
                            nullptr, nullptr, nullptr);
    NumBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, VideoCodecContext->width,
                                        VideoCodecContext->height, 1);
    OutBuffer = (const uchar*)(av_malloc(NumBytes*sizeof(unsigned char)));
    int res = av_image_fill_arrays(RgbFrame->data, RgbFrame->linesize, (const uint8_t*)OutBuffer,
                                   AV_PIX_FMT_RGB32, VideoCodecContext->width,
                                   VideoCodecContext->height, 1);
    if(res < 0)
    {
        qDebug() << "<Open> Fail to fill arrays";
        return false;
    }

    return true;
}

void player::run()
{
    if(!openFile())
    {
        qDebug() << "<OpenFile><Run> Can't open_yuv file";
        return ;
    }
    int res;
    while(StopFlag)
    {
        res = av_read_frame(FmtCtx, Pkt);
        if(res == AVERROR_EOF)
        {
            qDebug() << "<ReadFrame> Reached the file end";
            break;
        }
        else if(res < 0)
        {
            qDebug() << "<ReadFrame> Can't read frame, code:" << res;
            return ;
        }
        if(Pkt->stream_index == VideoStreamIndex)
        {
            res = avcodec_send_packet(VideoCodecContext, Pkt);
            if(res == AVERROR(EAGAIN))
            {
                qDebug() << "<SendPkt> Buffer Full";
            }
            else if(res == AVERROR(EOF))
            {
                qDebug() << "<SendPkt> Input File End";
            }
            else if(res < 0)
            {
                qDebug() << "<SendPkt> Fail to Send Packet to AVCodec";
                return ;
            }
            res = avcodec_receive_frame(VideoCodecContext, YuvFrame);
            if(res == AVERROR(EAGAIN))
            {
                qDebug() << "<ReceivePkt> No Output Data";
                continue;
            }
            else if(res == AVERROR_EOF)
            {
                qDebug() << "<ReceivePkt> Output File End";
                break;
            }
            else if(res < 0)
            {
                qDebug() << "<ReceivePkt> Fail to Receive Frame from AVCodec";
                return ;
            }
            // 进行格式转换
            sws_scale(ImgCtx, YuvFrame->data, YuvFrame->linesize,
                      0, VideoCodecContext->height, RgbFrame->data,
                      RgbFrame->linesize);
            QImage img(OutBuffer, VideoCodecContext->width,
                       VideoCodecContext->height, QImage::Format_RGB32);
            emit sendQImage(img);
            QThread::msleep(30);
        }
        // 播放声音的部分以后再写
        av_packet_unref(Pkt);
    }
    qDebug() << "<Player::Run> Video end";
}

AudioPlayer::~AudioPlayer()
{
    if(pIODevice->isOpen())
    {
        pAudioSink->stop();
        pIODevice->close();
    }
    if(!pkt) av_packet_free(&pkt);
    if(!pAudioCodecCtx) avcodec_free_context(&pAudioCodecCtx);
    if(!pAudioCodecCtx) avcodec_close(pAudioCodecCtx);
    if(!pAudioFmtCtx) avformat_close_input(&pAudioFmtCtx);
}

void AudioPlayer::setUrl(QString url)
{
    Url = std::move(url);
}

int32_t AudioPlayer::openFile()
{
    if(Url.isEmpty()) return -2;

    ret = avformat_open_input(&pAudioFmtCtx, Url.toLocal8Bit().data(), nullptr, nullptr);
    if(ret < 0)
    {
        qDebug() << "Can't open input file";
        return -1;
    }

    ret = avformat_find_stream_info(pAudioFmtCtx, nullptr);
    if(ret < 0)
    {
        qDebug() << "Can't find any stream info";
        return -1;
    }

    av_dump_format(pAudioFmtCtx, 0, Url.toLocal8Bit().data(), 0);

    auto StreamCnt = (int32_t)(pAudioFmtCtx->nb_streams);
    for(int32_t i = 0; i < StreamCnt; i++)
    {
        if(pAudioFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            AudioStreamIndex = i;
            continue;
        }
    }

    if(AudioStreamIndex == -1)
    {
        qDebug() << "Can't find any Stream";
        return -1;
    }

    // Audio
    pAudioCodecPara = pAudioFmtCtx->streams[AudioStreamIndex]->codecpar;
    pAudioCodec = const_cast<AVCodec*>(avcodec_find_decoder(pAudioCodecPara->codec_id));
    if(!pAudioCodec)
    {
        qDebug() << "Can't find codec";
    }

    pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
    if(!pAudioCodecCtx)
    {
        qDebug() << "Can't alloc codec context";
        return -1;
    }

    ret = avcodec_parameters_to_context(pAudioCodecCtx, pAudioCodecPara);
    if(ret < 0)
    {
        qDebug() << "Can't init parameters";
        return -1;
    }

    pAudioCodecCtx->pkt_timebase = pAudioFmtCtx->streams[AudioStreamIndex]->time_base;

    ret = avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr);
    if(ret < 0)
    {
        qDebug() << "Can't open audio codec";
        return -1;
    }

    AVChannelLayout OutChLayout = pAudioCodecCtx->ch_layout;
    OutSampleRate = pAudioCodecCtx->sample_rate;
    OutChannels = pAudioCodecCtx->ch_layout.nb_channels;

    Buffer = (uint8_t*)av_malloc(max_audio_frame_size*2);

    swr_alloc_set_opts2(&pSwrCtx, &OutChLayout, OutSampleFmt, OutSampleRate,
                        &(pAudioCodecCtx->ch_layout), pAudioCodecCtx->sample_fmt,
                        pAudioCodecCtx->sample_rate, 0, nullptr);

    ret = swr_init(pSwrCtx);
    if(ret)
    {
        qDebug() << "Can't init swr";
        return -1;
    }

    return 0;
}

void AudioPlayer::run()
{
    ret = openFile();
    if(ret < 0)
    {
        qDebug() << "Can't open file";
        return ;
    }

    int64_t SleepTime;
    while(av_read_frame(pAudioFmtCtx, pkt) >= 0)
    {
        if(pkt->stream_index != AudioStreamIndex)
            continue;
        ret = avcodec_send_packet(pAudioCodecCtx, pkt);
        if(ret < 0)
        {
            av_packet_unref(pkt);
            continue;
        }
        while(avcodec_receive_frame(pAudioCodecCtx, pAudioFrame) >= 0)
        {
            int length = 0;
            if(av_sample_fmt_is_planar(pAudioCodecCtx->sample_fmt))
            {
                length = swr_convert(pSwrCtx,
                                         &Buffer,
                                         (int)(max_audio_frame_size*2),
                                         (const uint8_t **)pAudioFrame->data,
                                         pAudioFrame->nb_samples);
                if(length < 0)
                    continue;
            }

            int OutSize = av_samples_get_buffer_size(nullptr, OutChannels, length,
                                                     OutSampleFmt, 1);

            SleepTime = (OutSampleRate*16*2)/8/OutSize;

            if(pAudioSink->bytesFree() < OutSize)
                msleep(SleepTime);
            pIODevice->write((const char*)Buffer, OutSize);
        }
        av_packet_unref(pkt);
    }
}
