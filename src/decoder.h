#include <iostream>

#ifdef _cplusplus
extern "C"
{
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#ifdef _cplusplus
}
#endif

/*
    AVFormatContext
        主要存储视音频封装格式中包含的信息 用于解封装
    AVStream
        存储一个视频/音频流的相关数据
    AVCodecContext
        每个AVStream对应一个AVCodecContext
        存储该视频/音频流使用解码方式的相关数据
    AVCodec
        每个AVCodecContext中对应一个AVCodec
        包含该视频/音频对应的解码器
        每种解码器都对应一个AVCodec结构
    AVPacket
        存放解码前数据
    AVFrame
        存放解码后数据
*/

namespace player
{

class decoder
{

private:
    AVFormatContext *pAVFormatContext;
    AVCodecContext  *pVideoDecodeContext;
    uint32_t        VideoStreamIndex;
    AVCodecContext  *pAudioDecodeContext;
    uint32_t        AudioStreamIndex;

public:
    decoder()
    {
        pAVFormatContext    = nullptr;
        pVideoDecodeContext = nullptr;
        pAudioDecodeContext = nullptr;
        VideoStreamIndex    = -1;
        AudioStreamIndex    = -1;
    }

public:
    int32_t open(const char *FilePath);
    void close();
    int32_t readFrame();
    static int32_t decode_packet_to_frame(AVCodecContext *pAVCodecContext, AVPacket *pAVPacket, AVFrame **pAVFrame);
    void enqueue(AVFrame *pAVFrame);

}; // class decoder

} // namespace decoder
