// cpp

// Qt

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

int main(int argc, char *argv[])
{
    auto* t1 = new test1();
    t1->test();
    delete t1;

    /*auto* t2 = new test2();
    t2->test();
    delete t2;*/

    return 0;
}
