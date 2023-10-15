#include "test.h"

#include <QDebug>

void test1::test()
{
    if(avformat_open_input(&fmt_cxt, file_name, nullptr, nullptr) < 0)
    {
        qDebug() << "<Open> Can't open_yuv file";
        avformat_close_input(&fmt_cxt);
        return ;
    }
    if(avformat_find_stream_info(fmt_cxt, nullptr) < 0)
    {
        qDebug() << "<Open> Can't find stream information";
        avformat_close_input(&fmt_cxt);
        return ;
    }
    av_dump_format(fmt_cxt, 0, file_name, 0);
    avformat_close_input(&fmt_cxt);
}

void test2::test()
{
    decoder->open(file_name);
    decoder->read_frame();
    decoder->close();
}

void test3::test()
{
    encoder->open_yuv(in_filename, out_filename);
    encoder->decode();
    encoder->close();
}

void test4::test()
{
    packager->open_h264(in_filename, out_filename);
    packager->package();
    packager->close();
}
