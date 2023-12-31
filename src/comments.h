#ifndef FFPLAYER_COMMENTS_H
#define FFPLAYER_COMMENTS_H

/*
 *  用到的struct:
 *
 *  AVFormatContext
 *      主要存储视音频封装格式中包含的信息 用于解封装
 *      AVIOContext *pb 输入数据的缓存
 *      unsigned int nb_streams  音视频流个数
 *      AVStream **streams  音视频流
 *      enum AVCodecID video_codec_id   视频格式ID
 *      enum AVCodecID audio_codec_id   音频格式ID
 *  AVStream
 *      存储一个视频/音频流的相关数据
 *      AVCodecParameters *codecpar 编解码器参数
 *      AVCodecContext *codec   指向该流的AVCodecContext
 *      AVRational time_base    时间基 将PTS,DTS转换为真正的时间
 *      AVRational avg_frame_rate   视频帧率
 *      int64_t duration    音视频流长度
 *  AVCodecParameters
 *      存储编解码器相关参数
 *      const struct AVCodec *codec 编解码器，初始化后即不可更改
 *      enum AVMediaType codec_type 编解码器类型
 *      enum AVCodecID codec_id 编解码器id
 *      int format  视频像素格式或音频采样格式
 *      int width   每帧视频宽度
 *      int height  每帧视频高度
 *      AVChannelLayout ch_layout   声道布局和声道数
 *  AVChannelLayout
 *      声道布局和声道数
 *      AVChannelOrder order    枚举，定义了音频通道的排列顺序
 *      int nb_channels 声道数
 *  AVCodecContext
 *      每个AVStream对应一个AVCodecContext
 *      存储该视频/音频流使用解码方式的相关数据
 *      pkt_timebase    帧率?
 *      AVChannelLayout ch_layout   声道布局和声道数
 *  AVCodec
 *      每个AVCodecContext中对应一个AVCodec
 *      包含该视频/音频对应的解码器
 *      每种解码器都对应一个AVCodec结构
 *  AVPacket
 *      存放解码前数据
 *      int stream_index    标识Packet所在的音视频流
 *  AVFrame
 *      存放解码后数据
 *      uint8_t* data   原始数据(RGB, YUV, PCM)
 *      int linesize[AV_NUM_DATA_POINTERS]  data中一行数据大小
 *      int width, height   视频的宽 高
 *      int format  帧格式
 *      int nb_samples  每个音频帧内包含的采样数
 *      int sample_rate 采样率
 *      AVChannelLayout ch_layout   声道布局和声道数
 *  AVInputFormat
 *  AVOutputFormat
 *      包含输入/输出文件容器格式
 *  AVRational
 *      表示有理数的结构
 *      AVRational.den表示分母
 *      AVRational.num表示分子
 *      AVStream.time_base是一个AVRational结构
 *      用于表示视频帧率
 * */

/* 用到的函数/方法
 *
 * int sws_scale(struct SwsContext* c, const uint8_t* const srcSlice[],
 *               const int srcStride[], int srcSliceY, int srcSliceH,
 *               uint8_t* const dst[], const int dstStride[])
 * c                    sws_get_context函数的返回值
 * srcSlice, dst        输入输出图象数据各颜色通道的buffer指针数组
 * srcStride, dstStride 输入输出图像数据各颜色通道每行存储的字节数数组
 * srcSliceY            输入图像开始扫描的列，通常为0
 * srcSliceH            输入共需扫描的行数，通常为输入图像的高度
 *
 *
 * int av_image_fill_arrays(uint8_t **dst_data, int *dst_linesize,
 *                          const uint8_t *src, AVPixelFormat pix_fmt,
 *                          int width, int height, int align)
 * dst_data     输出数据
 * dst_linesize 输出数据的高度
 * src          包含实际图像数据的Buffer
 * pix_fmt      图像的数据格式
 * width        图像宽度
 * height       图像高度
 * align        src中用于内存对齐的值，一般为1
 *              使用1即为按1字节对齐，得到的结果与原先数据相同
 *              如果为4则内存必须开始于4的倍数
 *
 * struct SwsContext sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
 *                                  int dstW, int dstH, enum AVPixelFormat dstFormat,
 *                                  int flags, SwsFilter* srcFilter, SwsFilter* dstFilter,
 *                                  const double* param)
 * srcW, srcH           输入图片宽 高
 * dstW, dstH           输出图片宽 高
 * srcFormat, dstFormat 输入 输出图片格式
 * flags                scale算法种类
 * */

#endif //FFPLAYER_COMMENTS_H
