//
// Created by Jacky on 2023/11/2.
//

#ifndef FFPLAYER_AUDIO_PLAYER_H
#define FFPLAYER_AUDIO_PLAYER_H


#include <iostream>

#include <QDebug>
#include <QFile>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <qtestsupport_core.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

const int MaxAudioFrameSize = 192000;

int Audio_player();

#endif //FFPLAYER_AUDIO_PLAYER_H
