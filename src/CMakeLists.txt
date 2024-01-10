find_package(Qt6 COMPONENTS
        Core
        Multimedia
        REQUIRED
)

include_directories(D:/Environment/ffmpeg/ffmpeg-master-latest-win64-gpl-shared/include)
link_directories(D:/Environment/ffmpeg/ffmpeg-master-latest-win64-gpl-shared/lib)
include_directories(D:/apps/Qt/6.5.3/mingw_64/include)
link_directories(D:/apps/Qt/6.5.3/mingw_64/lib)

aux_source_directory(. SRC)

add_library(src ${SRC}
        t_encoder.cpp
        t_encoder.h
        audio_player.cpp
        audio_player.h)

target_link_libraries(src
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        avdevice
        avfilter
        avformat
        avcodec
        avutil
        swresample
        postproc
        swscale
)
