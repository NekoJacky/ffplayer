#include "t_encoder.h"

namespace ff_player
{

    int32_t t_encode_yuv::open_yuv(const char *InFilePath, const char* OutFilePath)
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

    int32_t t_encode_yuv::encode()
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

    void t_encode_yuv::close()
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

    int32_t t_encode_yuv::flush_encoder()
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

    int32_t t_encode_pcm::open_pcm(const char *InFilePath, const char* OutFilePath)
    {
        InFile = fopen(InFilePath, "rb");
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
        pAudioStream = avformat_new_stream(pFmtCtx, nullptr);
        if(!pAudioStream)
        {
            qDebug() << "Can't crate output stream";
            return -1;
        }

        pAudioCodecPara = pFmtCtx->streams[pAudioStream->index]->codecpar;
        pAudioCodecPara->codec_type = AVMEDIA_TYPE_AUDIO;
        pAudioCodecPara->codec_id = pOutFmt->audio_codec;
        pAudioCodecPara->sample_rate = 44100;
        pAudioCodecPara->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
        pAudioCodecPara->bit_rate = 128000;
        pAudioCodecPara->format = AV_SAMPLE_FMT_FLTP;
        pAudioCodecPara->ch_layout.nb_channels = 2;

        pAudioCodec = const_cast<AVCodec*>(avcodec_find_encoder(pOutFmt->audio_codec));
        if(!pAudioCodec)
        {
            qDebug() << "Can't find codec";
            return -1;
        }
        pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
        avcodec_parameters_to_context(pAudioCodecCtx, pAudioCodecPara);
        if(!pAudioCodecCtx)
        {
            qDebug() << "Can't create codec context from codec para";
            return -1;
        }

        ret = avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr);
        if(ret < 0)
        {
            qDebug() << "Can't open codec";
            return  -1;
        }

        av_dump_format(pFmtCtx, 0, OutFilePath, 1);

        return 0;
    }

    int32_t t_encode_pcm::encode()
    {
        // 设置重采样参数
        pAudioFrame->nb_samples = pAudioCodecCtx->frame_size;
        pAudioFrame->format = pAudioCodecCtx->sample_fmt;
        pAudioFrame->ch_layout.nb_channels = 2;

        // PCM重采样
        struct SwrContext* pSwrCtx;
        swr_alloc_set_opts2(&pSwrCtx, &(pAudioCodecCtx->ch_layout),
                            pAudioCodecCtx->sample_fmt, pAudioCodecCtx->sample_rate,
                            &(pAudioFrame->ch_layout), AV_SAMPLE_FMT_FLTP, 44100, 0, nullptr);
        swr_init(pSwrCtx);

        uint8_t **convert_data;
        convert_data = (uint8_t**)calloc(pAudioCodecCtx->ch_layout.nb_channels, sizeof(*convert_data));
        av_samples_alloc(convert_data, nullptr, pAudioCodecCtx->ch_layout.nb_channels, pAudioCodecCtx->frame_size,
                         pAudioCodecCtx->sample_fmt, 0);
        size = av_samples_get_buffer_size(nullptr, pAudioCodecCtx->ch_layout.nb_channels,
                                          pAudioCodecCtx->frame_size, pAudioCodecCtx->sample_fmt, 1);
        Buffer = (uint8_t*)av_malloc(size);
        avcodec_fill_audio_frame(pAudioFrame, pAudioCodecCtx->ch_layout.nb_channels,
                                 pAudioCodecCtx->sample_fmt, Buffer, (int)size, 1);

        // 编码
        int32_t i = 0;
        while(true)
        {
            // 一帧数据的长度
            int length = pAudioFrame->nb_samples *
                         pAudioFrame->ch_layout.nb_channels *
                         av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
            // 读取pcm
            ret = (int)fread(Buffer, 1, length, InFile);
            if(ret < 0)
            {
                qDebug() << "Can't read input file";
                return -1;
            }
            else if(feof(InFile))
            {
                qDebug() << "End of infile";
                break;
            }
            swr_convert(pSwrCtx, convert_data, pAudioCodecCtx->frame_size,
                        (const uint8_t**)pAudioFrame->data, pAudioFrame->nb_samples);

            // 一帧数据长度
            length = pAudioCodecCtx->frame_size * av_get_bytes_per_sample(pAudioCodecCtx->sample_fmt);
            memcpy(pAudioFrame->data[0], convert_data[0], length);
            memcpy(pAudioFrame->data[1], convert_data[0], length);

            pAudioFrame->pts = i*100;
            ret = avcodec_send_frame(pAudioCodecCtx, pAudioFrame);
            if(ret < 0)
            {
                while(avcodec_receive_packet(pAudioCodecCtx, pkt) >= 0)
                {
                    pkt->stream_index = pAudioStream->index;
                    qDebug() << "write " << i << " frame, size=" << size << ", length=" << length;
                    av_write_frame(pFmtCtx, pkt);
                }
            }
            av_packet_unref(pkt);

            i++;
        }
        return 0;
    }

    void t_encode_pcm::close()
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
        if(pAudioCodecCtx)
        {
            avcodec_close(pAudioCodecCtx);
            avcodec_free_context(&pAudioCodecCtx);
            pAudioCodecCtx = nullptr;
        }
        if(pAudioFrame) av_free(pAudioFrame);
        if(Buffer) av_free(Buffer);
        fclose(InFile);
    }

    void t_encoder::encode(const char *YUVInFilePath, const char *YUVOutFilePath,
                           const char *PCMInFilePath, const char *PCMOutFilePath)
    {
        if(YUVInFilePath)
        {
            yuv_encoder.open_yuv(YUVInFilePath, YUVOutFilePath);
            yuv_encoder.encode();
            yuv_encoder.close();
        }
        if(PCMInFilePath)
        {
            pcm_encoder.open_pcm(PCMInFilePath, PCMOutFilePath);
            pcm_encoder.encode();
            pcm_encoder.close();
        }
    }

    int32_t t_packager::open_h264(const char *InFilePath, const char* OutFilePath)
    {
        // 打开输入文件
        ret = avformat_open_input(&pInFmtCtx, InFilePath, nullptr, nullptr);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't open input file";
            return -1;
        }
        ret = avformat_find_stream_info(pInFmtCtx, nullptr);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't find stream info";
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
            qDebug() << "<Packager> Can't find video stream";
            return -1;
        }
        pCodecPara = pInFmtCtx->streams[InVideoStreamIndex]->codecpar;
        av_dump_format(pInFmtCtx, 0, InFilePath, 0);
        // 设置视频帧率(test video frame rate)
        pInFmtCtx->streams[InVideoStreamIndex]->r_frame_rate = {30, 1};

        // 打开输出文件并填充格式数据
        ret = avformat_alloc_output_context2(&pOutFmtCtx, nullptr, nullptr, OutFilePath);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't crate output file";
            return -1;
        }
        // 打开输出文件并填充数据
        ret = avio_open(&pOutFmtCtx->pb, OutFilePath, AVIO_FLAG_READ_WRITE);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't open output file";
            return -1;
        }

        // 创建一个视频流
        pOutVideoStream = avformat_new_stream(pOutFmtCtx, nullptr);
        if(!pOutVideoStream)
        {
            qDebug() << "<Packager> Can't create video stream";
            return -1;
        }
        pOutVideoStream->time_base.den = 30;
        pOutVideoStream->time_base.num = 1;
        OutVideoStreamIndex = pOutVideoStream->index;

        pOutVideoCodec = const_cast<AVCodec*>(avcodec_find_encoder(pCodecPara->codec_id));
        if(!pOutVideoStream)
        {
            qDebug() << "<Packager> Can't find any encoder";
            return -1;
        }

        pOutVideoCodecContext = avcodec_alloc_context3(pOutVideoCodec);
        pOutVideoCodecPara = pOutFmtCtx->streams[OutVideoStreamIndex]->codecpar;
        ret = avcodec_parameters_copy(pOutVideoCodecPara, pCodecPara);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't copy parameters";
            return -1;
        }
        ret = avcodec_parameters_to_context(pOutVideoCodecContext, pOutVideoCodecPara);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't get codec context from parameters";
            return -1;
        }
        pOutVideoCodecContext->time_base.den = 30;
        pOutVideoCodecContext->time_base.num = 1;

        ret = avcodec_open2(pOutVideoCodecContext, pOutVideoCodec, nullptr);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't open output codec";
            return -1;
        }
        av_dump_format(pOutFmtCtx, 0, OutFilePath, 1);

        return 0;
    }

    int32_t t_packager::package()
    {
        ret = avformat_write_header(pOutFmtCtx, nullptr);
        if(ret < 0)
        {
            qDebug() << "<Packager> Can't write header";
            return -1;
        }
        pInVideoStream = pInFmtCtx->streams[InVideoStreamIndex];
        while(av_read_frame(pInFmtCtx, pPacket) >= 0)
        {
            if(pPacket->stream_index == InVideoStreamIndex)
            {
                if(pPacket->pts == AV_NOPTS_VALUE)
                {
                    AVRational time_base = pInVideoStream->time_base;
                    int64_t d = AV_TIME_BASE / av_q2d(pInVideoStream->r_frame_rate);
                    pPacket->pts = int64_t((double)(FrameIndex*d) / (double)(av_q2d(time_base)*AV_TIME_BASE));
                    pPacket->dts = pPacket->pts;
                    pPacket->duration = int64_t(double(d) / (av_q2d(time_base)*AV_TIME_BASE));
                    FrameIndex++;
                }
                pPacket->pts = av_rescale_q_rnd(pPacket->pts,
                                                pInVideoStream->time_base,
                                                pOutVideoStream->time_base,
                                                (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pPacket->dts = av_rescale_q_rnd(pPacket->dts,
                                                pInVideoStream->time_base,
                                                pOutVideoStream->time_base,
                                                (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pPacket->duration = av_rescale_q(pPacket->duration, pInVideoStream->time_base,
                                                 pOutVideoStream->time_base);
                pPacket->pos = -1;
                pPacket->stream_index = OutVideoStreamIndex;
                ret = av_interleaved_write_frame(pOutFmtCtx, pPacket);
                if(ret < 0)
                {
                    qDebug() << "<Packager> Error packet";
                    return -1;
                }
                av_packet_unref(pPacket);
            }
        }
        av_write_trailer(pOutFmtCtx);

        return 0;
    }

    void t_packager::close()
    {
        if(!pPacket)
        {
            av_packet_free(&pPacket);
            pPacket = nullptr;
        }
        if(!pInFmtCtx)
        {
            avformat_close_input(&pInFmtCtx);
            avformat_free_context(pInFmtCtx);
            pInFmtCtx = nullptr;
        }
        if(!pOutFmtCtx)
        {
            avformat_close_input(&pOutFmtCtx);
            avformat_free_context(pOutFmtCtx);
            pOutFmtCtx = nullptr;
        }
        if(!pOutVideoCodecContext)
        {
            avcodec_close(pOutVideoCodecContext);
            avcodec_free_context(&pOutVideoCodecContext);
            pOutVideoCodecContext = nullptr;
        }
    }

    void t_encoder_packager::encode_and_package(const char* YUVInFilePath, const char* YUVOutFilePath,
                                                const char* PCMInFilePath, const char* PCMOutFilepath)
    {
        encoder.encode(YUVInFilePath, "./temp.h264",
                       PCMInFilePath, PCMOutFilepath);
        packager.open_h264("./temp.h264", YUVOutFilePath);
        packager.package();
        packager.close();
        std::remove("./temp.h264");
    }
}
