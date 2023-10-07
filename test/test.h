// 测试类文件

#ifndef FFPLAYER_TEST_H
#define FFPLAYER_TEST_H

#include "../src/t_decoder.h"
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
    test2() { decoder = new ff_player::t_decoder(); file_name = "D:/Project/C/ffplayer/test/videos/test_mp4.mp4"; }
    ~test2() { delete decoder; file_name = nullptr; }

public:
    void test();
};

class test3
{
private:

public:

public:

};

#endif
