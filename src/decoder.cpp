#include "decoder.h"

// 打开并解析媒体文件
int32_t player::decoder::open(const char *FilePath)
{
    
    AVCodec *pVideoDecoder = nullptr;
    AVCodec *pAudioDecoder = nullptr;
    int     res            = 0;
    
    res = avformat_open_input(&pAVFormatContext, FilePath, nullptr, nullptr);
    if(pAVFormatContext == nullptr)
    {
        std::clog << "Can't Open the File" << std::endl;
        return res;
    }

    // 读取媒体流信息
    res = avformat_find_stream_info(pAVFormatContext, nullptr);
    if(res == AVERROR_EOF)
    {
        std::clog << "Reached File End" << std::endl;
        close();
        return -1;
    }

    // 遍历媒体流信息
    for(unsigned int i = 0; i < pAVFormatContext->nb_streams; i++)
    {
        AVStream *pAVStream = pAVFormatContext->streams[i];
        // 视频流
        if(pAVStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if((pAVStream->codecpar->width <= 0) || (pAVStream->codecpar->height <= 0))
            {
                std::clog << "Invalid Video Stream, Stream Index:" << i << std::endl;
                continue;
            }
            // 查找视频解码器
            pVideoDecoder = const_cast<AVCodec*>(avcodec_find_decoder(pAVStream->codecpar->codec_id));
            if(pVideoDecoder == nullptr)
            {
                std::clog << "Can't Find Video Codec" << std::endl;
                continue;
            }
            VideoStreamIndex = i;
            std::clog << "<Open> Format=" << pAVStream->codecpar->format << ", Frame Size="
                      << pAVStream->codecpar->width << "*" << pAVStream->codecpar->height << std::endl;
        }
        // 音频流
        else if(pAVStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if((pAVStream->codecpar->channels <= 0) || (pAVStream->codecpar->sample_rate <= 0))
            {
                std::clog << "Invalid Audeo Stream, Stream Index:" << i << std::endl;
                continue;
            }
            pAudioDecoder = const_cast<AVCodec*>(avcodec_find_decoder(pAVStream->codecpar->codec_id));
            if(pAudioDecoder == nullptr)
            {
                std::clog << "Can't find Audio Codec" <<std::endl;
                continue;
            }
            AudioStreamIndex = i;
            std::clog << "<Open> Format=" << pAVStream->codecpar->format << ", Frame Size="
                      << pAVStream->codecpar->width << "*" << pAVStream->codecpar->height << std::endl;
        }
    }

    if(pAudioDecoder == nullptr || pVideoDecoder == nullptr)
    {
        std::clog << "<Open> Can't find video or Audio Stream" << std::endl;
        close();
        return -1;
    }

    // seek到0ms读取
    res = avformat_seek_file(pAVFormatContext, -1, INT64_MIN, 0, INT64_MAX, 0);

    // 创建视频/音频解码器并打开
    if(pVideoDecoder != nullptr)
    {
        pVideoDecodeContext = avcodec_alloc_context3(pVideoDecoder);
        if(pVideoDecodeContext == nullptr)
        {
            std::clog << "Fail to Alloc a Video Decoder Context" << std::endl;
            close();
            return -1;
        }

        // 将编解码器参数从AVCodecContext中分离出来
        res = avcodec_parameters_to_context(
            pVideoDecodeContext,
            pAVFormatContext->streams[VideoStreamIndex]->codecpar
        );
        res = avcodec_open2(pVideoDecodeContext, nullptr, nullptr);
        if(res != 0)
        {
            std::clog << "Fail to Open a Video Codec, code " << res << std::endl;
            close();
            return -1;
        }
    }
    if(pAudioDecoder != nullptr)
    {
        pAudioDecodeContext = avcodec_alloc_context3(pAudioDecoder);
        if(pAudioDecodeContext == nullptr)
        {
            std::clog << "Fail to Alloc a Audio Decoder Context" << std::endl;
            close();
            return -1;
        }

        res = avcodec_parameters_to_context(
            pAudioDecodeContext,
            pAVFormatContext->streams[AudioStreamIndex]->codecpar
        );
        res = avcodec_open2(pAudioDecodeContext, nullptr, nullptr);
        if(res != 0)
        {
            std::clog << "Fail to Open a Audio Codec, code " << res << std::endl;
            close();
            return -1;
        }
    }

    return 0;
}

void player::decoder::close()
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

// 读取音视频包进行解码
int32_t player::decoder::readFrame()
{
    int res = 0;
    while(true)
    {
        AVPacket* pAVPacket = av_packet_alloc();

        // 读取数据包
        res = av_read_frame(pAVFormatContext, pAVPacket);
        if(res == AVERROR_EOF)
        {
            std::clog << "Reached File End" << std::endl;
            break;
        }
        else if(res < 0)
        {
            std::clog << "Fail to Read Frame , code " << res << std::endl;
            break; 
        }

        // 调用decode_packet_to_frame()进行解码
        if(pAVPacket->stream_index == VideoStreamIndex)
        {
            AVFrame *pVideoFrame = nullptr;
            res = decode_packet_to_frame(pVideoDecodeContext, pAVPacket, &pVideoFrame);
            if(res == 0 && pVideoFrame != nullptr)
            {
                enquene(pVideoFrame);
            }
        }
        else if(pAVPacket->stream_index == AudioStreamIndex)
        {
            AVFrame *pAudioFrame = nullptr;
            res = decode_packet_to_frame(pAudioDecodeContext, pAVPacket, &pAudioFrame);
            if(res == 0 && pAudioFrame != nullptr)
            {
                enquene(pAudioFrame);
            }
        }

        av_packet_free(&pAVPacket);
    }
    return 0;
}

/* 解码器核心方法 */
/*
    avcodec_send_packet() 和 avcodec_receive_frame() 通常是同时使用的
    先调用 avcodec_send_packet() 送入要解码的数据包
    然后调用 avcodec_receive_frame()获取解码后的音视频数据
        这两个函数是异步的，内部有缓冲区
        因此可能出现缓冲区满或者缓冲区无内容的情况

        通常解码开始，通过avcodec_send_packet()送入几十个数据包
        对应的avcodec_receive_frame()都没有音视频帧输出
        等送入的数据包足够多后，avcodec_receive_frame()才开始输出前面一开始送入进行解码的音视频帧
        最后几十没有数据包送入了，也要调用avcodec_send_packet()送入空数据包，以驱动解码模块继续解码缓冲区中的数据
        此时avcodec_receive_frame()还是会有音视频帧输出
        直到返回AVERROR_EOF才表示所有数据包解码完成。
*/
int32_t player::decoder::decode_packet_to_frame(AVCodecContext *pDecoderContext, AVPacket *pInPacket, AVFrame **ppOutFrame)
{
    AVFrame *pOutFrame  = nullptr;
    int     res         = 0;
    
    // 开始解码
    res = avcodec_send_packet(pDecoderContext, pInPacket);
    if(res == AVERROR(EAGAIN))
    {
        std::clog << "Buffer Full" << std::endl;
    }
    else if(res == AVERROR(EOF))
    {
        std::clog << "Input File End" << std::endl;
    }
    else if(res < 0)
    {
        std::clog << "Fail to Send Packet to AVCodec" << std::endl;
    }

    // 获取解码后的视频或音频帧
    pOutFrame = av_frame_alloc();
    res = avcodec_receive_frame(pDecoderContext, pOutFrame);
    if(res == AVERROR(EAGAIN))
    {
        std::clog << "No Output Data" << std::endl;
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return 0;
    }
    else if(res == AVERROR_EOF)
    {
        std::clog << "Output File End" << std::endl;
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return AVERROR_EOF;
    }
    else if(res < 0)
    {
        std::clog << "Fali to Receive Frame from AVCodec" << std::endl;
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return res;
    }

    (*ppOutFrame) = pOutFrame;
    return 0;
}

void player::decoder::enquene(AVFrame *pAVFrame)
{
    // ...
}
