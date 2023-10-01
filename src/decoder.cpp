#include "decoder.h"

#include <utility>

decoder::decoder()
{
    file_is_open = false;
    fmt_cxt = avformat_alloc_context();
    VideoCodec = nullptr;
    VideoCodecContext = nullptr;
    pkt = av_packet_alloc();
    RgbFrame = av_frame_alloc();
    YuvFrame = av_frame_alloc();
    img_cxt = nullptr;
    out_buffer = nullptr;
    VideoStreamIndex = -1;
    NumBytes = -1;
    Url = "";
}

decoder::~decoder()
{
    if(!fmt_cxt) avformat_close_input(&fmt_cxt);
    if(!VideoCodecContext) avcodec_close(VideoCodecContext);
    if(!VideoCodecContext) avcodec_free_context(&VideoCodecContext);
    if(!pkt) av_packet_free(&pkt);
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
}
