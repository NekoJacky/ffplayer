#include "decoder.h"

decoder::decoder()
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
}

decoder::~decoder()
{
    if(!FmtCtx) avformat_close_input(&FmtCtx);
    if(!VideoCodecContext) avcodec_close(VideoCodecContext);
    if(!VideoCodecContext) avcodec_free_context(&VideoCodecContext);
    if(!Pkt) av_packet_free(&Pkt);
    if(!RgbFrame) av_frame_free(&RgbFrame);
    if(!YuvFrame) av_frame_free(&YuvFrame);
}

void decoder::setUrl(QString url)
{
    Url = std::move(url);
}

bool decoder::openFile()
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
                            AV_PIX_FMT_RGB24, SWS_BICUBIC,
                            nullptr, nullptr, nullptr);
    NumBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, VideoCodecContext->width,
                                        VideoCodecContext->height, 1);
    OutBuffer = (char*)(av_malloc(NumBytes*sizeof(unsigned char)));
    int res = av_image_fill_arrays(RgbFrame->data, RgbFrame->linesize, (const uint8_t*)OutBuffer,
                                   AV_PIX_FMT_RGB24, VideoCodecContext->width,
                                   VideoCodecContext->height, 1);
    if(res < 0)
    {
        qDebug() << "<Open> Fail to fill arrays";
        return false;
    }

    return true;
}

void decoder::run()
{
    if(!openFile())
    {
        qDebug() << "<Decoder><Run> Can't open file";
        return ;
    }
}
