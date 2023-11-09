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
#include <libswresample/swresample.h>
#ifdef __cplusplus
}
#endif

namespace ff_player
{

/* ff_player::t_encoder
 * yuv->h.264
 * */
class t_encode_yuv
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
    t_encode_yuv()
    {
        pFmtCtx             = nullptr;
        pOutFmt             = nullptr;
        pVideoStream        = nullptr;
        pVideoCodecCtx      = nullptr;
        pVideoCodec         = nullptr;
        pkt                 = nullptr; // av_packet_alloc();
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
     * @param OutFilePath   解码文件路径
     * @retval 0    成功打开文件并编码
     * @retval -1   具体错误请查看控制台输出信息
     * */
    int32_t open_yuv(const char* InFilePath, const char* OutFilePath);

    /**
     * @brief 解码并写入h.264文件
     * @retval 0    成功打开文件并编码
     * @retval -1   具体错误请查看控制台输出信息
     * */
    int32_t encode();

    /**
     * @brief 关闭所有的编码相关struct，关闭文件
     * */
    void close();

protected:
    /**
     * @brief 刷新编码器
     * */
    int32_t flush_encoder();
};

/* ff_player::t_encode_pcm
 * pcm->mp3
 * */
class t_encode_pcm
{
private:
    AVFormatContext     *pFmtCtx;
    AVOutputFormat      *pOutFmt;
    AVStream            *pAudioStream;
    AVCodecContext      *pAudioCodecCtx;
    AVCodec             *pAudioCodec;
    AVCodecParameters   *pAudioCodecPara;
    AVPacket            *pkt;
    AVFrame             *pAudioFrame;
    uint8_t             *Buffer;
    size_t              size;
    int32_t             ret;
    int32_t             AudioStreamIndex;
    FILE                *InFile;

public:
    t_encode_pcm(): pFmtCtx(avformat_alloc_context()),
                    pOutFmt(nullptr),
                    pAudioStream(nullptr),
                    pAudioCodecCtx(nullptr),
                    pAudioCodec(nullptr),
                    pAudioCodecPara(nullptr),
                    pkt(av_packet_alloc()),
                    pAudioFrame(av_frame_alloc()),
                    Buffer(nullptr),
                    size(0),
                    ret(-1),
                    AudioStreamIndex(-1),
                    InFile(nullptr) {}

public:
    /**
     * @brief 打开文件，打开编码器
     * @param InFilePath    输入文件路径
     * @param OutFilePath   输出文件路径
     * @retval 0    成功打开文件与编码器
     * @retval -1   出现错误，具体错误请打开控制台查看
     * */
    int32_t open_pcm(const char* InFilePath, const char* OutFilePath);

    int32_t encode();

    void close();
};

class t_encoder
{
private:
    t_encode_yuv yuv_encoder;
    t_encode_pcm pcm_encoder;
public:
    t_encoder(): yuv_encoder(t_encode_yuv()), pcm_encoder(t_encode_pcm()) {}
public:
    void encode(const char* YUVInFilePath, const char* YUVOutFilePath,
                const char* PCMInFilePath, const char* PCMOutFilePath);
};

/* ff_player::t_packager
 * h.264->mp4
 * */
class t_packager
{
private:
    AVFormatContext*    pInFmtCtx;
    AVFormatContext*    pOutFmtCtx;
    AVCodecParameters*  pCodecPara;
    AVStream*           pOutVideoStream;
    AVCodec*            pOutVideoCodec;
    AVCodecContext*     pOutVideoCodecContext;
    AVCodecParameters*  pOutVideoCodecPara;
    AVStream*           pInVideoStream;
    AVPacket*           pPacket;
    int32_t             FrameIndex;
    int32_t             InVideoStreamIndex;
    int32_t             OutVideoStreamIndex;
    int                 ret;
public:
    t_packager():
            pInFmtCtx(nullptr),
            pOutFmtCtx(nullptr),
            pCodecPara(nullptr),
            pOutVideoStream(nullptr),
            pOutVideoCodec(nullptr),
            pOutVideoCodecContext(nullptr),
            pOutVideoCodecPara(nullptr),
            pInVideoStream(nullptr),
            pPacket(av_packet_alloc()),
            FrameIndex(0),
            InVideoStreamIndex(-1),
            OutVideoStreamIndex(-1),
            ret(-1)
    {}
public:
    /**
     * @brief 打开并编码原始文件
     * @param InFilePath    原始文件路径
     * @param OutFilePath   封装文件路径
     * @retval 0    成功打开文件并编码
     * @retval -1   具体错误请查看控制台输出信息
     * */
    int32_t open_h264(const char* InFilePath, const char* OutFilePath);

    /**
     * @brief 将h.264等视频文件与mp3等音频文件打包为目标(mp4)文件
     * @retval 0    成功打包文件
     * @retval -1   具体错误查看控制台输出信息
     * */
    int32_t package();
    void close();
};

class t_encoder_packager
{
private:
    t_encoder encoder;
    t_packager packager;
public:
    t_encoder_packager() = default;
    ~t_encoder_packager() = default;
public:
    void encode_and_package(const char* YUVInFilePath, const char* YUVOutFilePath,
                            const char* PCMInFilePath, const char* PCMOutFilepath);
};

}

#endif //FFPLAYER_ENCODER_H
