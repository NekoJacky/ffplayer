// 测试类文件

#ifndef FFPLAYER_TEST_H
#define FFPLAYER_TEST_H

#include "../src/t_decoder.h"
#include "../src/t_encoder.h"
#include "../src/audio_player.h"
#include "../widget.h"

#include <QString>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif

// 简单测试，输出测试视频的信息
class test1
{
private:
    AVFormatContext* fmt_cxt;
    int ret;
    const char* file_name;

public:
    test1(): fmt_cxt(avformat_alloc_context()), ret(0),
        file_name("D:/Project/C/ffplayer/test/videos/test_mp4.mp4") {}
    explicit test1(const char* input_file_name): fmt_cxt(avformat_alloc_context()), ret(0),
        file_name(input_file_name) {}

public:
    void test();
};

// 简单测试ff_player::decoder的功能
class test2
{
private:
    ff_player::t_decoder* decoder;
    const char* file_name;

public:
    test2() { decoder = new ff_player::t_decoder(); file_name = R"(D:\Project\C\ffplayer\test\audios\test_mp3.mp3)"; }
    ~test2() { delete decoder; file_name = nullptr; }

public:
    void test();
};

class test3
{
private:
    ff_player::t_encode_yuv* encoder;
    const char* in_filename;
    const char* out_filename;
public:
    test3():
        encoder(new ff_player::t_encode_yuv()),
        in_filename(R"(D:\Project\C\ffplayer\test\videos\test_yuv.yuv)"),
        out_filename(R"(D:\Project\C\ffplayer\test\videos\test_h264.h264)")
    {}
    ~test3() { delete encoder; }
public:
    void test();
};

class test4
{
private:
    ff_player::t_packager* packager;
    const char* in_filename;
    const char* out_filename;
public:
    test4(): packager(new ff_player::t_packager()),
        in_filename(R"(D:\Project\C\ffplayer\test\videos\test_h264.h264)"),
        out_filename(R"(D:\Project\C\ffplayer\test\videos\test_mp4_1.mp4)") {}
    ~test4() { delete packager; }
public:
    void test();
};

class test5
{
public:
    test5() = default;
    ~test5() = default;
    void test();
};

#endif
