#include "t_encoder.h"

namespace ff_player
{

    int32_t t_encoder::open_yuv(const char *InFilePath, const char* OutFilePath)
    {
        int32_t w = 1920;
        int32_t h = 1080;

        // 打开输入文件
        InFile = fopen(InFilePath, "rb");

        // 打开输出文件
        ret = avformat_alloc_output_context2(&pFmtCtx, nullptr, nullptr, OutFilePath);
        if(ret < 0)
        {
            qDebug() << "Can't alloc output context";
            return -1;
        }
        pOutFmt = const_cast<AVOutputFormat*>(pFmtCtx->oformat);
        ret = avio_open(&pFmtCtx->pb, OutFilePath, AVIO_FLAG_READ_WRITE);
        if(ret < 0)
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
        pVideoStream->avg_frame_rate.num = 30;
        pVideoStream->avg_frame_rate.den = 1;

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
        pVideoCodecCtx->codec_id        = pOutFmt->video_codec;
        pVideoCodecCtx->codec_type      = AVMEDIA_TYPE_VIDEO;
        pVideoCodecCtx->pix_fmt         = AV_PIX_FMT_YUV420P;
        pVideoCodecCtx->width           = w;
        pVideoCodecCtx->height          = h;
        pVideoCodecCtx->time_base.num   = 1;
        pVideoCodecCtx->time_base.den   = 30;
        pVideoCodecCtx->bit_rate        = 4000000;
        pVideoCodecCtx->gop_size        = 12;
        if(pVideoCodecCtx->codec_id == AV_CODEC_ID_H264)
        {
            pVideoCodecCtx->qmin = 10;
            pVideoCodecCtx->qmax = 51;
            pVideoCodecCtx->qcompress = 0.6;
        }
        if(pVideoCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
        {
            pVideoCodecCtx->max_b_frames = 2;
        }
        if(pVideoCodecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
        {
            pVideoCodecCtx->mb_decision = 2;
        }

        // 打开编码器
        ret = avcodec_open2(pVideoCodecCtx, pVideoCodec, nullptr);
        if(ret < 0)
        {
            qDebug() << "Can't open_yuv video codec";
            return -1;
        }

        // 输出文件流信息
        av_dump_format(pFmtCtx, 0, OutFilePath, 1);

        return 0;
    }

    int32_t t_encoder::decode()
    {
        // test
        int32_t FrameCnt = 2847;

        // 初始化帧
        pVideoFrame = av_frame_alloc();
        pVideoFrame->width = pVideoCodecCtx->width;
        pVideoFrame->height = pVideoCodecCtx->height;
        pVideoFrame->format = pVideoCodecCtx->pix_fmt;
        size = av_image_get_buffer_size(pVideoCodecCtx->pix_fmt,
                                        pVideoCodecCtx->width,
                                        pVideoCodecCtx->height,
                                        1);
        Buffer = (uint8_t*)av_malloc(size);
        av_image_fill_arrays(pVideoFrame->data,
                             pVideoFrame->linesize,
                             Buffer,
                             pVideoCodecCtx->pix_fmt,
                             pVideoCodecCtx->width,
                             pVideoCodecCtx->height,
                             1);

        // 写头文件
        ret = avformat_write_header(pFmtCtx, nullptr);
        if(ret < 0)
        {
            qDebug() << "Can't write header";
            return -1;
        }

        int32_t y_size = pVideoCodecCtx->width * pVideoCodecCtx->height;
        av_new_packet(pkt, (int)(size*3));

        pVideoFrame->pts = 0;
        // 编码帧
        for(int32_t i = 0; i < FrameCnt; i++)
        {
            ret = (int32_t)fread(Buffer, 1, y_size * 3 / 2, InFile);
            if(ret <= 0)
            {
                qDebug() << "Can't read file";
                return -1;
            }
            else if(feof(InFile))
                break;
            pVideoFrame->data[0] = Buffer;
            pVideoFrame->data[1] = Buffer+y_size;
            pVideoFrame->data[2] = Buffer+y_size*5/4;
            pVideoFrame->pts++;

            if(avcodec_send_frame(pVideoCodecCtx, pVideoFrame) >= 0)
            {
                while(avcodec_receive_packet(pVideoCodecCtx, pkt) >= 0)
                {
                    pkt->stream_index = pVideoStream->index;
                    av_packet_rescale_ts(pkt, pVideoCodecCtx->time_base,
                                         pVideoStream->time_base);
                    pkt->pos = -1;
                    ret = av_interleaved_write_frame(pFmtCtx, pkt);
                    if(ret < 0)
                    {
                        qDebug() << "Write frame error";
                    }
                    av_packet_unref(pkt);
                }
            }
        }

        VideoStreamIndex = pVideoStream->index;
        ret = flush_encoder();
        if(ret < 0)
        {
            qDebug() << "Can't flush encoder";
            return -1;
        }

        av_write_trailer(pFmtCtx);

        return 0;
    }

    void t_encoder::close()
    {
        if(pFmtCtx) {
            avio_close(pFmtCtx->pb);
            avformat_free_context(pFmtCtx);
            pFmtCtx = nullptr;
        }
        if(pOutFmt) pOutFmt = nullptr;
        if(pkt)
        {
            av_packet_free(&pkt);
            pkt = nullptr;
        }
        if(pVideoCodecCtx)
        {
            avcodec_close(pVideoCodecCtx);
            avcodec_free_context(&pVideoCodecCtx);
            pVideoCodecCtx = nullptr;
        }
        if(pVideoFrame) av_free(pVideoFrame);
        if(Buffer) av_free(Buffer);
        fclose(InFile);
    }

    int32_t t_encoder::flush_encoder()
    {
        int32_t f_ret;
        AVPacket *f_pkt = av_packet_alloc();
        f_pkt->data = nullptr;
        f_pkt->size = 0;

        if(!(pVideoCodecCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
            return 0;

        if(avcodec_send_frame(pVideoCodecCtx, nullptr) >= 0)
        {
            while(avcodec_receive_packet(pVideoCodecCtx, f_pkt) >= 0)
            {
                f_pkt->stream_index = VideoStreamIndex;
                av_packet_rescale_ts(f_pkt, pVideoCodecCtx->time_base,
                                     pFmtCtx->streams[VideoStreamIndex]->time_base);
                f_ret = av_interleaved_write_frame(pFmtCtx, f_pkt);
                if(f_ret < 0)
                    break;
            }
        }
        return ret;
    }

    int32_t t_packager::open_h264(const char *InFilePath, const char* OutFilePath)
    {
        // 打开输入文件
        InFile = fopen(InFilePath, "rb");
        ret = avformat_open_input(&pInFmtCtx, InFilePath, nullptr, nullptr);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't open input file";
            return -1;
        }
        ret = avformat_find_stream_info(pInFmtCtx, nullptr);
        if(ret < 0)
        {
            qDebug() << "Can't find stream info";
            return -1;
        }
        for(int i = 0; i < pInFmtCtx->nb_streams; i++)
        {
            if(pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                InVideoStreamIndex = i;
                break;
            }
        }
        if(InVideoStreamIndex == -1)
        {
            qDebug() << "Can't find video stream";
            return -1;
        }
        pCodecPara = pInFmtCtx->streams[InVideoStreamIndex]->codecpar;
        av_dump_format(pInFmtCtx, 0, InFilePath, 0);

        // 打开输出文件并填充格式数据
        ret = avformat_alloc_output_context2(&pOutFmtCtx, nullptr, nullptr, OutFilePath);
        if(ret < 0)
        {
            qDebug() << "Can't crate output file";
            return -1;
        }
        // 打开输出文件并填充数据
        ret = avio_open(&pOutFmtCtx->pb, OutFilePath, AVIO_FLAG_READ_WRITE);
        if(ret < 0)
        {
            qDebug() << "Can't open output file";
            return -1;
        }

        // 创建一个视频流
        pOutVideoStream = avformat_new_stream(pOutFmtCtx, nullptr);
        if(!pOutVideoStream)
        {
            qDebug() << "Can't create video stream";
            return -1;
        }
        pOutVideoStream->time_base.den = 30;
        pOutVideoStream->time_base.num = 1;
        OutVideoStreamIndex = pOutVideoStream->index;

        pOutVideoCodec = const_cast<AVCodec*>(avcodec_find_encoder(pCodecPara->codec_id));
        if(!pOutVideoStream)
        {
            qDebug() << "Can't find any encoder";
            return -1;
        }

        pOutVideoCodecContext = avcodec_alloc_context3(pOutVideoCodec);
        pOutVideoCodecPara = pOutFmtCtx->streams[OutVideoStreamIndex]->codecpar;
        ret = avcodec_parameters_copy(pOutVideoCodecPara, pCodecPara);
        if(ret < 0)
        {
            qDebug() << "Can't copy parameters";
            return -1;
        }
        ret = avcodec_parameters_to_context(pOutVideoCodecContext, pOutVideoCodecPara);
        if(ret < 0)
        {
            qDebug() << "Can't get codec context from parameters";
            return -1;
        }
        pOutVideoCodecContext->time_base.den = 30;
        pOutVideoCodecContext->time_base.num = 1;

        ret = avcodec_open2(pOutVideoCodecContext, pOutVideoCodec, nullptr);
        if(ret < 0)
        {
            qDebug() << "Can't open output codec";
            return -1;
        }
        av_dump_format(pOutFmtCtx, 0, OutFilePath, 1);

        return 0;
    }

    int32_t t_packager::package()
    {
        return 0;
    }

    void t_packager::close()
    {

    }

    void t_encoder_packager::encode_and_packge()
    {

    }
}
