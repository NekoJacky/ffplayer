#include "t_encoder.h"

namespace ff_player
{

int32_t t_encoder::open(const char *InFilePath)
{
    int32_t w = 1920;
    int32_t h = 1080;
    // 打开输入文件
    FILE *InFile = fopen(InFilePath, "rb");

    // 打开输出文件
    const char*OutFilePath = R"(D:\Project\C\ffplayer\test\videos\test_h264.h264)";
    res = avformat_alloc_output_context2(&pFmtCtx, nullptr, nullptr, OutFilePath);
    if(res < 0)
    {
        qDebug() << "Can't alloc output context";
        return -1;
    }
    pOutFmt = const_cast<AVOutputFormat*>(pFmtCtx->oformat);
    res = avio_open(&pFmtCtx->pb, OutFilePath, AVIO_FLAG_READ_WRITE);
    if(res < 0)
    {
        qDebug() << "Can't open output file";
        return -1;
    }

    // 创建h.264视频流
    pVideoStream = avformat_new_stream(pFmtCtx, pVideoCodec);
    if(!pVideoStream)
    {
        qDebug() << "Can't crate new video stream";
        return -1;
    }
    pVideoStream->time_base.den = 30;
    pVideoStream->time_base.num = 1;

    AVCodecParameters *pCodecPara = pFmtCtx->streams[pVideoStream->index]->codecpar;
    pCodecPara->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecPara->width = w;
    pCodecPara->height = h;

    // 查找编码器
    pVideoCodec = const_cast<AVCodec*>(avcodec_find_encoder(pOutFmt->video_codec));
    if(!pVideoCodec)
    {
        qDebug() << "Can't find encoder";
        return -1;
    }

    // 设置编码器内容
    pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
    avcodec_parameters_to_context(pVideoCodecCtx, pCodecPara);
    if(!pVideoCodecCtx)
    {
        qDebug() << "Can't alloc codec context";
        return -1;
    }

    return 0;
}

void t_encoder::close()
{

}

}
