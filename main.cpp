// cpp

// Qt
#include <QApplication>

// ffmpeg
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/ffversion.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif

#include "test/test.h"
#include "widget.h"
#include "src/t_decoder.h"
#include "src/audio_player.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Widget w;
    w.setWindowTitle("ffplayer");
    w.show();

    return QApplication::exec();

    /*mp4->yuv*/
    /*auto *a = new ff_player::t_decoder();
    a->open("D:/Project/C/ffplayer/test/audios/test_mp3.mp3");
    a->read_frame();
    a->close();
    delete a;*/

    /* yuv->h.264 */
    /*auto* t = new test3();
    t->test();
    delete t;*/

    /* h.264->mp4*/
    /*auto t = new test4();
    t->test();
    delete t;*/

    /*test5 t;
    t.test();*/
}
