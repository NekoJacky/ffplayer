/* ff_player::t_encoder
 * yuv->h.264: encode
 * h.264->mp4: package
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
    int32_t         res;

public:
    t_encoder()
    {
        pFmtCtx         = nullptr;
        pOutFmt         = nullptr;
        pVideoStream    = nullptr;
        pVideoCodecCtx  = nullptr;
        pVideoCodec     = nullptr;
        pkt             = av_packet_alloc();
        pVideoFrame     = nullptr;
        Buffer          = nullptr;
        size            = 0;
        res             = -1;
    }

public:
    /**
     * @brief 打开并编码原始文件
     * @param InFilePath    原始文件路径
     * @retval 0    成功打开文件并编码
     * @retval -1   具体错误请查看控制台输出信息
     * */
    int32_t open(const char* InFilePath);
    void close();
};

}

#endif //FFPLAYER_ENCODER_H
