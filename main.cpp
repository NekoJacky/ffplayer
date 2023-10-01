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
#include "src/player_widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget w;
    w.setWindowTitle("ffplayer");
    w.show();

    return QApplication::exec();
}
