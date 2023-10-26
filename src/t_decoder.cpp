#include "t_decoder.h"

int32_t ff_player::t_decoder::open(const char *InFilePath)
{
    AVCodec *pVideoDecoder = nullptr;
    AVCodec *pAudioDecoder = nullptr;
    int     ret;
    
    avformat_open_input(&pAVFormatContext, InFilePath, nullptr, nullptr);
    if(pAVFormatContext == nullptr)
    {
        qDebug() << "Can't Open the File";
        return -1;
    }

    // 读取媒体流信息
    ret = avformat_find_stream_info(pAVFormatContext, nullptr);
    if(ret == AVERROR_EOF)
    {
        qDebug() << "Reached File End";
        close();
        return -1;
    }

    av_dump_format(pAVFormatContext, 0, InFilePath, 0);

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

    /*if(pVideoDecoder == nullptr)
    {
        qDebug() << "<Open> Can't find video Stream";
        close();
        return -1;
    }
    if(pAudioDecoder == nullptr)
    {
        qDebug() << "<Open> Can't find audio Stream";
        close();
        return -1;
    }*/

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
    ret = avcodec_open2(pVideoDecodeContext, nullptr, nullptr);
    if(ret != 0)
    {
        qDebug() << "Fail to Open a Video Codec, code " << ret;
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
    pAudioDecodeContext->pkt_timebase =
            pAVFormatContext->streams[AudioStreamIndex]->time_base;
    ret = avcodec_open2(pAudioDecodeContext, nullptr, nullptr);
    if(ret != 0)
    {
        qDebug() << "Fail to Open a Audio Codec, code " << ret;
        close();
        return -1;
    }

    return 0;
}

void ff_player::t_decoder::close()
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

int32_t ff_player::t_decoder::read_frame()
{
    int ret;
    int i = 0;
    int j = 0;
    while(true)
    {
        AVPacket* pAVPacket = av_packet_alloc();

        // 读取数据包
        ret = av_read_frame(pAVFormatContext, pAVPacket);
        if(ret == AVERROR_EOF)
        {
            qDebug() << "Reached File End";
            break;
        }
        else if(ret < 0)
        {
            qDebug() << "Fail to Read Frame , code " << ret;
            break; 
        }

        // 调用decode_packet_to_frame()进行解码
        if(pAVPacket->stream_index == VideoStreamIndex)
        {
            AVFrame *pVideoFrame = nullptr;
            /* mp4->yuv
            w = pVideoDecodeContext->width;
            h = pVideoDecodeContext->height;
             */
            ret = decode_packet_to_frame(pVideoDecodeContext, pAVPacket, &pVideoFrame);
            if(ret == 0 && pVideoFrame != nullptr)
            {
                i++;
                // std::clog << i << std::endl;
                /* mp4->yuv
                fwrite(pVideoFrame->data[0], 1, w*h, File);
                fwrite(pVideoFrame->data[1], 1, (w*h)/4, File);
                fwrite(pVideoFrame->data[2], 1, (w*h)/4, File);

                fflush(File);
                 */
            }
        }
        else if(pAVPacket->stream_index == AudioStreamIndex)
        {
            AVFrame *pAudioFrame = nullptr;
            ret = decode_packet_to_frame(pAudioDecodeContext, pAVPacket, &pAudioFrame);
            if(ret == 0 && pAudioFrame != nullptr)
            {
                j++;
                // std::clog << j << std::endl;
                /* mp3->pcm */
                // planner: 平面，数据以nb_samples采样点交错
                // 数据排列方式为LLLLLLRRRRRRLLLLLLRRRRRR...（每组LLLLLLRRRRRR为一个音频帧）
                // 不带P的音频格式为LRLR...每个LR为一个音频样本
                ret = av_sample_fmt_is_planar(pAudioDecodeContext->sample_fmt);
                if(!ret)
                    continue;
                int NBytes = av_get_bytes_per_sample(pAudioDecodeContext->sample_fmt);
                // pcm格式播放时为LRLR...形式，因此要交错保存
                for(int k = 0; k < pAudioFrame->nb_samples; k++)
                {
                    for(int ch = 0; ch < pAudioDecodeContext->channels; ch++)
                    {
                        fwrite(pAudioFrame->data[ch]+NBytes*i, 1, NBytes, File);
                    }
                }
            }
        }

        av_packet_free(&pAVPacket);
    }
    qDebug() << i << " frames in total";
    return 0;
}

int32_t ff_player::t_decoder::decode_packet_to_frame(AVCodecContext *pDecoderContext,
                                                     AVPacket *pInPacket, AVFrame **ppOutFrame)
{
    AVFrame *pOutFrame  = nullptr;
    int     ret;
    
    // 开始解码
    ret = avcodec_send_packet(pDecoderContext, pInPacket);
    if(ret == AVERROR(EAGAIN))
    {
        qDebug() << "Buffer Full";
    }
    else if(ret == AVERROR(EOF))
    {
        qDebug() << "Input File End";
    }
    else if(ret < 0)
    {
        qDebug() << "Fail to Send Packet to AVCodec";
    }

    // 获取解码后的视频或音频帧
    pOutFrame = av_frame_alloc();
    ret = avcodec_receive_frame(pDecoderContext, pOutFrame);
    if(ret == AVERROR(EAGAIN))
    {
        qDebug() << "No Output Data";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return 0;
    }
    else if(ret == AVERROR_EOF)
    {
        qDebug() << "Output File End";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return AVERROR_EOF;
    }
    else if(ret < 0)
    {
        qDebug() << "Fail to Receive Frame from AVCodec";
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return ret;
    }

    (*ppOutFrame) = pOutFrame;
    return 0;
}
