#include "player.h"

player::player()
{
    FmtCtx = avformat_alloc_context();
    VideoCodec = nullptr;
    VideoCodecContext = nullptr;
    AudioCodec = nullptr;
    AudioCodecContext = nullptr;
    Pkt = av_packet_alloc();
    RgbFrame = av_frame_alloc();
    YuvFrame = av_frame_alloc();
    ImgCtx = nullptr;
    OutBuffer = nullptr;
    VideoStreamIndex = -1;
    AudioStreamIndex = -1;
    NumBytes = -1;
    Url = "";
    StopFlag = true;
}

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
        qDebug() << "<Open> Can't open video codec";
        return false;
    }
    if(avcodec_open2(AudioCodecContext, AudioCodec, nullptr) < 0)
    {
        qDebug() << "<Open> Can't open audio codec";
        return false;
    }

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
        qDebug() << "<OpenFile><Run> Can't open file";
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