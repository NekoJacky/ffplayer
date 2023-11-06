// Jacky 2023-10-28

#include "audio_player.h"

int audio_player()
{
    QString InFileUrl = R"(D:\Project\C\ffplayer\test\audios\test_mp3.mp3)";
    QAudioSink *audioSink;
    QIODevice *streamOut;

    QAudioFormat audioFmt;
    audioFmt.setSampleRate(44100);
    audioFmt.setChannelCount(2);
    /*audioFmt.setSampleSize(16);
    audioFmt.setCodec("audio/pcm");
    audioFmt.setByteOrder(QAudioFormat::LittleEndian);
    audioFmt.setSampleType(QAudioFormat::SignedInt);*/

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if(!device.isFormatSupported(audioFmt)){
        audioFmt = device.preferredFormat();
    }
    audioSink = new QAudioSink(audioFmt);
    audioSink->setVolume(100);

    streamOut = audioSink->start();

    AVFormatContext *fmtCtx =avformat_alloc_context();
    AVCodecContext *codecCtx = nullptr;
    AVPacket *pkt=av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int aStreamIndex = -1;

    if (avformat_open_input(&fmtCtx, InFileUrl.toLocal8Bit().data(), nullptr, nullptr) < 0)
    {
        qDebug("Cannot open input file.");
        return -1;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        qDebug("Cannot find any stream in file.");
        return -1;
    }
    av_dump_format(fmtCtx, 0, InFileUrl.toLocal8Bit().data(), 0);
    for (size_t i = 0; i < fmtCtx->nb_streams; i++)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            aStreamIndex = (int) i;
            break;
        }
    }
    if (aStreamIndex == -1)
    {
        qDebug("Cannot find audio stream.");
        return -1;
    }
    AVCodecParameters *aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
    auto codec = const_cast<AVCodec *>(avcodec_find_decoder(aCodecPara->codec_id));
    if (!codec)
    {
        qDebug("Cannot find any codec for audio.");
        return -1;
    }
    codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecCtx, aCodecPara) < 0)
    {
        qDebug("Cannot alloc codec context.");
        return -1;
    }
    codecCtx->pkt_timebase = fmtCtx->streams[aStreamIndex]->time_base;
    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        qDebug("Cannot open audio codec.");
        return -1;
    }//设置转码参数

    av_frame_free(&frame);
    auto out_channel_layout = codecCtx->ch_layout;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = codecCtx->sample_rate;
    int out_channels = out_channel_layout.nb_channels;
    //printf("out rate : %d , out_channel is: %d\n",out_sample_rate,out_channels);
    av_packet_free(&pkt);
    /* SwrContext   音频重采样结构体 类似于SwsContext */
    auto *audio_out_buffer = (uint8_t *) av_malloc(MaxAudioFrameSize * 2);
    avcodec_close(codecCtx);
    SwrContext *swr_ctx = nullptr;
    swr_alloc_set_opts2(&swr_ctx,
                        &out_channel_layout,
                        out_sample_fmt,
                        out_sample_rate,
                        &codecCtx->ch_layout,
                        codecCtx->sample_fmt,
                        codecCtx->sample_rate,
                        0, nullptr);
    swr_init(swr_ctx);
    double sleep_time = 0;
    while (av_read_frame(fmtCtx, pkt) >= 0)
    {
        if (pkt->stream_index == aStreamIndex)
        {
            if (avcodec_send_packet(codecCtx, pkt) >= 0)
            {
                while (avcodec_receive_frame(codecCtx, frame) >= 0)
                {
                    if (av_sample_fmt_is_planar(codecCtx->sample_fmt))
                    {
                        int len = swr_convert(swr_ctx,
                                              &audio_out_buffer,
                                              MaxAudioFrameSize * 2,
                                              (const uint8_t **) frame->data,
                                              frame->nb_samples);
                        if (len <= 0)
                        {
                            continue;
                        }
                        //qDebug("convert length is: %d.\n",len);

                        int out_size = av_samples_get_buffer_size(nullptr,
                                                                  out_channels,
                                                                  len,
                                                                  out_sample_fmt,
                                                                  1);
                        //qDebug("buffer size is: %d.",dst_bufsize);

                        sleep_time = ((double)out_sample_rate * 16 * 2 / 8) / out_size;

                        if (audioSink->bytesFree() < out_size)
                        {
                            QTest::qSleep((int)sleep_time);
                            streamOut->write((char *) audio_out_buffer, out_size);
                        } else
                        {
                            streamOut->write((char *) audio_out_buffer, out_size);
                        }
                        //将数据写入PCM文件
                        //fwrite(audio_out_buffer,1,dst_bufsize,file);
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }
    avcodec_free_context(&codecCtx);
    avformat_free_context(fmtCtx);

    streamOut->close();

    return 0;
}
