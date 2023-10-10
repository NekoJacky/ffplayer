/* ff_player::t_encoder
 * yuv->h.264: encode
 * h.264->mp4: package
 * 暂时只做视频编码相关
 * */

#ifndef FFPLAYER_ENCODER_H
#define FFPLAYER_ENCODER_H

#include <fstream>

#include <QDebug>
#include <QString>

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

/* AVInputFormat
 * AVOutputFormat
 *      包含输入/输出文件容器格式
 * AVRational
 *      表示有理数的结构
 *      AVRational.den表示分母
 *      AVRational.num表示分子
 *      AVStream.time_base是一个AVRational结构
 *      用于表示视频帧率
 * */

namespace ff_player
{

class t_encoder
{
private:
    AVFormatContext *pFmtCtx;
    AVOutputFormat  *pOutFmt;
    AVStream        *pVideoStream;
    AVCodecContext  *pVideoCodecCtx;
    AVCodec         *pVideoCodec;
    AVPacket        *pkt;
    AVFrame         *pVideoFrame;
    uint8_t         *Buffer;
    size_t          size;
    int32_t         ret;
    int32_t         VideoStreamIndex;
    FILE            *InFile;
public:
    t_encoder()
    {
        pFmtCtx             = nullptr;
        pOutFmt             = nullptr;
        pVideoStream        = nullptr;
        pVideoCodecCtx      = nullptr;
        pVideoCodec         = nullptr;
        pkt                 = av_packet_alloc();
        pVideoFrame         = nullptr;
        Buffer              = nullptr;
        size                = 0;
        ret                 = -1;
        VideoStreamIndex    = -1;
        InFile              = nullptr;
    }

public:
    /**
     * @brief 打开并编码原始文件
     * @param InFilePath    原始文件路径
     * @retval 0    成功打开文件并编码
     * @retval -1   具体错误请查看控制台输出信息
     * */
    int32_t open(const char* InFilePath);

    /**
     * @brief 关闭所有的编码相关struct，关闭文件
     * */
    void close();

    /**
     * @brief 刷新编码器
     * */
    int32_t flush_encoder();
};

}

#endif //FFPLAYER_ENCODER_H
