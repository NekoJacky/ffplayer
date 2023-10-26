/* ff_player::t_decoder
 * 一个简单的解码器
 * 简单学习一下解码操作
 * */

#ifndef FF_PLAYER_DECODER_H_
#define FF_PLAYER_DECODER_H_

#include <iostream>
#include <filesystem>
#include <string>

#include <QDebug>
#include <QString>
#include <fstream>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

/*
用到的struct:
    AVFormatContext
        主要存储视音频封装格式中包含的信息 用于解封装
        AVIOContext *pb 输入数据的缓存
        unsigned int nb_streams  音视频流个数
        AVStream **streams  音视频流
        enum AVCodecID video_codec_id   视频格式ID
        enum AVCodecID audio_codec_id   音频格式ID
    AVStream
        存储一个视频/音频流的相关数据
        AVCodecParameters *codecpar 编解码器参数
        AVCodecContext *codec   指向该流的AVCodecContext
        AVRational time_base    时间基 将PTS,DTS转换为真正的时间
        AVRational avg_frame_rate   视频帧率
        int64_t duration    音视频流长度
    AVCodecParameters
        存储编解码器相关参数
        const struct AVCodec *codec 编解码器，初始化后即不可更改
        enum AVMediaType codec_type 编解码器类型
        enum AVCodecID codec_id 编解码器id
        int format  视频像素格式或音频采样格式
        int width   每帧视频宽度
        int height  每帧视频高度
        AVChannelLayout ch_layout   声道布局和声道数
            int nb_channels 声道数
    AVCodecContext
        每个AVStream对应一个AVCodecContext
        存储该视频/音频流使用解码方式的相关数据
        pkt_timebase    帧率?
    AVCodec
        每个AVCodecContext中对应一个AVCodec
        包含该视频/音频对应的解码器
        每种解码器都对应一个AVCodec结构
    AVPacket
        存放解码前数据
        stream_index    标识Packet所在的音视频流
    AVFrame
        存放解码后数据
*/

namespace ff_player
{

class t_decoder
{

private:
    AVFormatContext *pAVFormatContext;
    AVCodecContext  *pVideoDecodeContext;
    uint32_t        VideoStreamIndex;
    AVCodecContext  *pAudioDecodeContext;
    uint32_t        AudioStreamIndex;
    /* mp4->yuv
    FILE            *File;
    int             w;
    int             h;
     */
    /* mp3->pcm */
    FILE            *File;

public:
    t_decoder()
    {
        pAVFormatContext    = nullptr;
        pVideoDecodeContext = nullptr;
        pAudioDecodeContext = nullptr;
        VideoStreamIndex    = -1;
        AudioStreamIndex    = -1;
        /* mp4->yuv
        File                = fopen(R"(D:\Project\C\ffplayer\test\videos\test_yuv.yuv)", "w+b");
        w                   = 0;
        h                   = 0;
         */
        /* mp3->pcm */
        File                = fopen(R"(D:\Project\C\ffplayer\test\audios\test_pcm.pcm)", "w+b");
    }

public:
    /**
     * @brief 打开并解析媒体文件
     * 注意，open_yuv()函数只用来解封装，解码媒体文件使用read_frame()
     * @param InFilePath  需要解析的文件的路径
     * @retval 0    成功打开并解析文件
     * @retval -1   出现错误，具体错误查看控制台中输出的信息
     * */
    int32_t open(const char *InFilePath);

    /**
     * @brief 关闭打开的媒体文件，释放系统资源
     * */
    void close();

    /**
     * @brief 读取音视频包，调用decode_packet_to_frame进行解码
     * @retval 0 读取结束
     * */
    int32_t read_frame();

    /**
     * @brief 解码器核心方法
     * avcodec_send_packet() 和 avcodec_receive_frame() 通常是同时使用的
     * 先调用 avcodec_send_packet() 送入要解码的数据包
     * 然后调用 avcodec_receive_frame()获取解码后的音视频数据
     *      这两个函数是异步的，内部有缓冲区
     *      因此可能出现缓冲区满或者缓冲区无内容的情况
     *
     *      通常解码开始，通过avcodec_send_packet()送入几十个数据包
     *      对应的avcodec_receive_frame()都没有音视频帧输出
     *      等送入的数据包足够多后，avcodec_receive_frame()才开始输出前面一开始送入进行解码的音视频帧
     *      最后几十没有数据包送入了，也要调用avcodec_send_packet()送入空数据包，以驱动解码模块继续解码缓冲区中的数据
     *      此时avcodec_receive_frame()还是会有音视频帧输出
     *      直到返回AVERROR_EOF才表示所有数据包解码完成。
     * @param pDecoderContext   输入数据包的相关解码数据
     * @param pInPacket         需要解码的数据包
     * @param ppOutFrame        用于接收解码后数据
     * @retval 0                成功解码
     * @retval AVERROR(EAGAIN)  输入端：缓冲区已满，无法接收新的数据
     * @retval AVERROR(EAGAIN)  输出端：输出不可用，需要更多输入才可以输出
     * @retval AVERROR(EOF)     输入端：到达输入文件末尾
     * @retval AVERROR_EOF      输出端：到达输出文件末尾，所有数据包解码完成
     * @retval -1               无法获得输出流
     * */
    static int32_t decode_packet_to_frame(AVCodecContext *pDecoderContext, AVPacket *pInPacket, AVFrame **ppOutFrame);

}; // class decoder

} // namespace decoder

#endif
