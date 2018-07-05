//
// Created by zz on 2018/7/5.
//

//#ifndef FFMPEGAPP_FFMPEGMUSIC_H
//#define FFMPEGAPP_FFMPEGMUSIC_H
//
//#endif //FFMPEGAPP_FFMPEGMUSIC_H
#ifndef FFMPEGMUSIC_FFMPEGMUSIC_H
#define FFMPEGMUSIC_FFMPEGMUSIC_H

#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"LC",FORMAT,##__VA_ARGS__);

int createFFmpeg(int *rate, int *channel, const char *string);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();
#endif