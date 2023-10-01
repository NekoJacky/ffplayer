/* ff_player::decoder
 * 解码器 */

#ifndef FF_PLAYER_DECODER_H_
#define FF_PLAYER_DECODER_H_

#include <filesystem>
#include <string>

#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

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

namespace ff_player
{

class decoder
{
public:
    struct frame_saver
    {
        struct SwsContext* ImgCtx;
        AVFrame* RgbFrame;
        AVFrame* YuvFrame;

        frame_saver() = default;
        explicit frame_saver(AVCodecContext* pVideoDecoderContext)
        {
            RgbFrame = av_frame_alloc();
            YuvFrame = av_frame_alloc();

            /* 设置数据转换参数
             * 前三个参数为源地址长宽以及数据格式
             * 然后的三个参数为目的地址长宽以及数据格式
             * 算法类型  AV_PIX_FMT_YUVJ420P   AV_PIX_FMT_BGR24
             * */
            ImgCtx = sws_getContext(pVideoDecoderContext->width, pVideoDecoderContext->height,
                                    pVideoDecoderContext->pix_fmt, pVideoDecoderContext->width,
                                    pVideoDecoderContext->height, AV_PIX_FMT_RGB32, SWS_BICUBIC,
                                    nullptr, nullptr, nullptr);

            // 分配一帧图像的数据大小
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, pVideoDecoderContext->width,
                                                    pVideoDecoderContext->height, 1);
            auto *out_buffer = (unsigned char *)av_malloc(numBytes * sizeof(unsigned char));

            // 将rgb_frame中的数据关联到pVideoDecoderContext，数据会同步改变
            av_image_fill_arrays(RgbFrame->data, RgbFrame->linesize, out_buffer, AV_PIX_FMT_RGB32,
                                 pVideoDecoderContext->width, pVideoDecoderContext->height, 1);
        }

        ~frame_saver()
        {
            av_frame_free(&RgbFrame);
            av_frame_free(&YuvFrame);
        }

        static frame_saver get_frame_saver(AVCodecContext* pVideoDecoderContext);

        /**
         * @brief 保存视频帧到file_path
         * @param pVideoFrame   要保存的视频帧
         * @param width         视频帧宽度
         * @param height        视频帧高度
         * @param frame         视频帧编号
         * @param file_path     保存路径
         * */
        static void save_video_frame_(AVFrame* pVideoFrame, int width, int height, int frame, const char* file_path);

        /**
         * @brief 保存视频帧到默认路径
         * 调用save_video_frame(AVFrame* pVideoFrame, int width, int height, int frame, const char* file_path)
         * */
        static void save_video_frame_test(AVFrame* pVideoFrame, int width, int height, int frame);

        /**
         * @brief 保存一帧图像
         * @param pVideoDecoderContext  视频帧的相关数据
         * @param i                     视频帧个数
         * */
        void save_video_frame(AVCodecContext* pVideoDecoderContext, int32_t i) const;
    };

private:
    AVFormatContext *pAVFormatContext;
    AVCodecContext  *pVideoDecodeContext;
    uint32_t        VideoStreamIndex;
    AVCodecContext  *pAudioDecodeContext;
    uint32_t        AudioStreamIndex;
    frame_saver     saver{};

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
    /**
     * @brief 打开并解析媒体文件
     * 注意，open()函数只用来解封装，解码媒体文件请使用read_frame()
     * @param FilePath  需要解析的文件的路径
     * @retval 0    成功打开并解析文件
     * @retval -1   出现错误，具体错误查看控制台中输出的信息
     * */
    int32_t open(const char *FilePath);

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
     * @param i                 视频帧数目
     * @retval 0                成功解码
     * @retval AVERROR(EAGAIN)  输入端：缓冲区已满，无法接收新的数据
     * @retval AVERROR(EAGAIN)  输出端：输出不可用，需要更多输入才可以输出
     * @retval AVERROR(EOF)     输入端：到达输入文件末尾
     * @retval AVERROR_EOF      输出端：到达输出文件末尾，所有数据包解码完成
     * @retval -1               无法获得输出流
     * */
    int32_t decode_packet_to_frame(AVCodecContext *pDecoderContext, AVPacket *pInPacket, AVFrame **ppOutFrame, int i);

}; // class decoder

} // namespace decoder

#endif
