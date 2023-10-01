#include "decoder.h"

int32_t ff_player::decoder::open(const char *FilePath)
{
    
    AVCodec *pVideoDecoder = nullptr;
    AVCodec *pAudioDecoder = nullptr;
    int     res;
    
    res = avformat_open_input(&pAVFormatContext, FilePath, nullptr, nullptr);
    if(pAVFormatContext == nullptr)
    {
        qDebug() << "Can't Open the File";
        return -1;
    }

    // 读取媒体流信息
    res = avformat_find_stream_info(pAVFormatContext, nullptr);
    if(res == AVERROR_EOF)
    {
        qDebug() << "Reached File End";
        close();
        return -1;
    }

    av_dump_format(pAVFormatContext, 0, FilePath, 0);

    // 遍历媒体流信息
    for(unsigned int i = 0; i < pAVFormatContext->nb_streams; i++)
    {
        AVStream *pAVStream = pAVFormatContext->streams[i];
        // 视频流
        if(pAVStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if((pAVStream->codecpar->width <= 0) || (pAVStream->codecpar->height <= 0))
            {
                qDebug() << "Invalid Video Stream, Stream Index:" << i;
                continue;
            }
            // 查找视频解码器
            pVideoDecoder = const_cast<AVCodec*>(avcodec_find_decoder(pAVStream->codecpar->codec_id));
            if(pVideoDecoder == nullptr)
            {
                qDebug() << "Can't Find Video Codec";
                continue;
            }
            VideoStreamIndex = i;
            qDebug() << "<Open> Format=" << pAVStream->codecpar->format << ", Frame Size="
                      << pAVStream->codecpar->width << "*" << pAVStream->codecpar->height;
        }
        // 音频流
        else if(pAVStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if((pAVStream->codecpar->ch_layout.nb_channels <= 0) || (pAVStream->codecpar->sample_rate <= 0))
            {
                qDebug() << "Invalid Audio Stream, Stream Index:" << i;
                continue;
            }
            pAudioDecoder = const_cast<AVCodec*>(avcodec_find_decoder(pAVStream->codecpar->codec_id));
            if(pAudioDecoder == nullptr)
            {
                qDebug() << "Can't find Audio Codec";
                continue;
            }
            AudioStreamIndex = i;
            qDebug() << "<Open> Format=" << pAVStream->codecpar->format << ", Frame Size="
                      << pAVStream->codecpar->width << "*" << pAVStream->codecpar->height;
        }
    }

    if(pAudioDecoder == nullptr || pVideoDecoder == nullptr)
    {
        qDebug() << "<Open> Can't find video or Audio Stream";
        close();
        return -1;
    }

    // seek到0ms读取
    avformat_seek_file(pAVFormatContext, -1, INT64_MIN, 0, INT64_MAX, 0);

    // 创建视频/音频解码器并打开
    pVideoDecodeContext = avcodec_alloc_context3(pVideoDecoder);
    if(pVideoDecodeContext == nullptr)
    {
        qDebug() << "Fail to Alloc a Video Decoder Context";
        close();
        return -1;
    }
    // 将编解码器参数从AVCodecContext中分离出来
    avcodec_parameters_to_context(
        pVideoDecodeContext,
        pAVFormatContext->streams[VideoStreamIndex]->codecpar
    );
    res = avcodec_open2(pVideoDecodeContext, nullptr, nullptr);
    if(res != 0)
    {
        qDebug() << "Fail to Open a Video Codec, code " << res;
        close();
        return -1;
    }

    pAudioDecodeContext = avcodec_alloc_context3(pAudioDecoder);
    if(pAudioDecodeContext == nullptr)
    {
        qDebug() << "Fail to Alloc a Audio Decoder Context";
        close();
        return -1;
    }
    avcodec_parameters_to_context(
        pAudioDecodeContext,
        pAVFormatContext->streams[AudioStreamIndex]->codecpar
    );
    res = avcodec_open2(pAudioDecodeContext, nullptr, nullptr);
    if(res != 0)
    {
        qDebug() << "Fail to Open a Audio Codec, code " << res;
        close();
        return -1;
    }

    return 0;
}

void ff_player::decoder::close()
{
    if(pAVFormatContext != nullptr)
    {
        avformat_close_input(&pAVFormatContext);
        pAVFormatContext = nullptr;
    }

    if(pVideoDecodeContext != nullptr)
    {
        avcodec_close(pVideoDecodeContext);
        avcodec_free_context(&pVideoDecodeContext);
        pVideoDecodeContext = nullptr;
    }

    if(pAudioDecodeContext != nullptr)
    {
        avcodec_close(pAudioDecodeContext);
        avcodec_free_context(&pAudioDecodeContext);
        pAudioDecodeContext = nullptr;
    }
}

int32_t ff_player::decoder::read_frame()
{
    int res;
    int i = 0;
    while(true)
    {
        AVPacket* pAVPacket = av_packet_alloc();

        // 读取数据包
        res = av_read_frame(pAVFormatContext, pAVPacket);
        if(res == AVERROR_EOF)
        {
            qDebug() << "Reached File End";
            break;
        }
        else if(res < 0)
        {
            qDebug() << "Fail to Read Frame , code " << res;
            break; 
        }

        // 调用decode_packet_to_frame()进行解码
        if(pAVPacket->stream_index == VideoStreamIndex)
        {
            AVFrame *pVideoFrame = nullptr;
            res = decode_packet_to_frame(pVideoDecodeContext, pAVPacket, &pVideoFrame);
            if(res == 0 && pVideoFrame != nullptr)
            {
                i++;
                // std::clog << '1' << std::endl;
            }
        }
        else if(pAVPacket->stream_index == AudioStreamIndex)
        {
            AVFrame *pAudioFrame = nullptr;
            res = decode_packet_to_frame(pAudioDecodeContext, pAVPacket, &pAudioFrame);
            if(res == 0 && pAudioFrame != nullptr)
            {
                // std::clog << '2' << std::endl;
            }
        }

        av_packet_free(&pAVPacket);
    }
    qDebug() << i << " frames in total";
    return 0;
}

int32_t ff_player::decoder::decode_packet_to_frame(AVCodecContext *pDecoderContext,
                                                   AVPacket *pInPacket, AVFrame **ppOutFrame)
{
    AVFrame *pOutFrame  = nullptr;
    int     res;
    
    // 开始解码
    res = avcodec_send_packet(pDecoderContext, pInPacket);
    if(res == AVERROR(EAGAIN))
    {
        qDebug() << "Buffer Full";
    }
    else if(res == AVERROR(EOF))
    {
        qDebug() << "Input File End";
    }
    else if(res < 0)
    {
        qDebug() << "Fail to Send Packet to AVCodec";
    }

    // 获取解码后的视频或音频帧
    pOutFrame = av_frame_alloc();
    res = avcodec_receive_frame(pDecoderContext, pOutFrame);
    if(res == AVERROR(EAGAIN))
    {
        qDebug() << "No Output Data";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return 0;
    }
    else if(res == AVERROR_EOF)
    {
        qDebug() << "Output File End";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return AVERROR_EOF;
    }
    else if(res < 0)
    {
        qDebug() << "Fail to Receive Frame from AVCodec";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return res;
    }

    (*ppOutFrame) = pOutFrame;
    return 0;
}