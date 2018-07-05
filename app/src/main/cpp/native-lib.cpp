#include <jni.h>
#include <string>
#include <android/log.h>

extern "C"{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <android/native_window_jni.h>
#include <unistd.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}
#include "FFmpegMusic.h"
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"LC",FORMAT,##__VA_ARGS__);
extern "C"
JNIEXPORT void JNICALL
Java_com_grechur_ffmpegapp_VideoView_render(JNIEnv *env, jclass type, jstring input_,
                                            jobject surface) {
    const char *input = env->GetStringUTFChars(input_, 0);
    //注册各大组件
//    av_register_all();
//    LOGE("注册成功")
    //获取上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    int error;
    char buff[] = "";
    //打开视频地址并获取里面的内容(解封装)

    if ((error = avformat_open_input(&avFormatContext, input, NULL, NULL)) < 0) {
        av_strerror(error,buff,1024);
        // LOGE("%s" ,inputPath)
        LOGE("Couldn't open file %s: %d(%s)", input, error, buff);
        // LOGE("%d",error)
        LOGE("打开视频失败")
        return;
    }
    if(avformat_find_stream_info(avFormatContext,NULL)<0){
        LOGE("获取内容失败")
        return;
    }
    //获取到整个内容过后找到里面的视频流
    int video_index=-1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        //过时avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
        if(avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index = i;
        }
    }
    LOGE("成功找到视频流")
    //对视频流进行解码
    AVCodecParameters *avCodecParameters = avFormatContext->streams[video_index]->codecpar;
    //获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
    //获取解码器上下文
    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(avCodecContext,avCodecParameters);
    //打开解码器
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGE("打开失败")
        return;
    }

    //申请AVPacket
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    //申请AVFrame
    AVFrame *frame = av_frame_alloc();//分配一个AVFrame结构体,AVFrame结构体一般用于存储原始数据，指向解码后的原始帧
    AVFrame *rgb_frame = av_frame_alloc();//分配一个AVFrame结构体，指向存放转换成rgb后的帧
    //输出文件
    //FILE *fp = fopen(outputPath,"wb");


    //缓存区
    uint8_t  *out_buffer= (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                                                  avCodecContext->width,avCodecContext->height,1));
    //与缓存区相关联，设置rgb_frame缓存区
//    avpicture_fill((AVPicture *)rgb_frame,out_buffer,AV_PIX_FMT_RGBA,avCodecContext->width,avCodecContext->height);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize,out_buffer,AV_PIX_FMT_RGBA,avCodecContext->width,avCodecContext->height,1);

    SwsContext* swsContext = sws_getContext(avCodecContext->width,avCodecContext->height,avCodecContext->pix_fmt,
                                            avCodecContext->width,avCodecContext->height,AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC,NULL,NULL,NULL);

    //取到nativewindow
    ANativeWindow *nativeWindow=ANativeWindow_fromSurface(env,surface);
    if(nativeWindow==0){
        LOGE("nativewindow取到失败")
        return;
    }
    //视频缓冲区
    ANativeWindow_Buffer native_outBuffer;


    int frameCount;
    int h =0;
    LOGE("解码 ")
    while (av_read_frame(avFormatContext, packet) >= 0) {
        LOGE("解码 %d",packet->stream_index)
        LOGE("VINDEX %d",video_index)
        if(packet->stream_index==video_index){
            LOGE("解码 hhhhh")
            //如果是视频流
            //解码
//            avcodec_decode_video2(avCodecContext, frame, &frameCount, packet);
            avcodec_send_packet(avCodecContext, packet);
            frameCount = avcodec_receive_frame(avCodecContext, frame);
            LOGE("解码中....  %d",frameCount)
            if (!frameCount) {
                LOGE("转换并绘制")
                //说明有内容
                //绘制之前配置nativewindow
                ANativeWindow_setBuffersGeometry(nativeWindow,avCodecContext->width,avCodecContext->height,WINDOW_FORMAT_RGBA_8888);
                //上锁
                ANativeWindow_lock(nativeWindow, &native_outBuffer, NULL);
                //转换为rgb格式
                sws_scale(swsContext,(const uint8_t *const *)frame->data,frame->linesize,0,
                          frame->height,rgb_frame->data,
                          rgb_frame->linesize);
                //  rgb_frame是有画面数据
                uint8_t *dst= (uint8_t *) native_outBuffer.bits;
//            拿到一行有多少个字节 RGBA
                int destStride=native_outBuffer.stride*4;
                //像素数据的首地址
                uint8_t * src=  rgb_frame->data[0];
//            实际内存一行数量
                int srcStride = rgb_frame->linesize[0];
                //int i=0;
                for (int i = 0; i < avCodecContext->height; ++i) {
//                memcpy(void *dest, const void *src, size_t n)
                    //将rgb_frame中每一行的数据复制给nativewindow
                    memcpy(dst + i * destStride,  src + i * srcStride, srcStride);
                }
//解锁
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 16);

            }
        }
//        av_free_packet(packet);
        av_packet_unref(packet);
    }
    //释放
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(input_, input);
}
//extern "C"
//JNIEXPORT void JNICALL
//Java_com_grechur_ffmpegapp_MusicPlay_playSound(JNIEnv *env, jobject instance, jstring input_) {
//    const char *input = env->GetStringUTFChars(input_, 0);
//
//    AVFormatContext *pFormatCtx = avformat_alloc_context();
//    //open
//    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
//        LOGE("%s","打开输入视频文件失败");
//        return;
//    }
//    //获取视频信息
//    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
//        LOGE("%s","获取视频信息失败");
//        return;
//    }
//    int audio_stream_idx=-1;
//    int i=0;
//    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
//        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
//            LOGE("  找到音频id %d", pFormatCtx->streams[i]->codecpar->codec_type);
//            audio_stream_idx=i;
//            break;
//        }
//    }
//    //对视频流进行解码
//    AVCodecParameters *avCodecParameters = pFormatCtx->streams[audio_stream_idx]->codecpar;
//    //获取解码器
//    AVCodec *pCodex = avcodec_find_decoder(avCodecParameters->codec_id);
//    //获取解码器上下文
//    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodex);
//    avcodec_parameters_to_context(pCodecCtx,avCodecParameters);
////    //获取解码器上下文
////    AVCodecContext *pCodecCtx=pFormatCtx->streams[audio_stream_idx]->codec;
////    //获取解码器
////    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
//    //打开解码器
//    if (avcodec_open2(pCodecCtx, pCodex, NULL)<0) {
//    }
//    //申请avpakcet，装解码前的数据
//    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
//    //申请avframe，装解码后的数据
//    AVFrame *frame = av_frame_alloc();
//    //得到SwrContext ，进行重采样，具体参考http://blog.csdn.net/jammg/article/details/52688506
//    SwrContext *swrContext = swr_alloc();
//    //缓存区
//    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
////输出的声道布局（立体声）
//    uint64_t  out_ch_layout=AV_CH_LAYOUT_STEREO;
////输出采样位数  16位
//    enum AVSampleFormat out_formart=AV_SAMPLE_FMT_S16;
////输出的采样率必须与输入相同
//    int out_sample_rate = pCodecCtx->sample_rate;
//
////swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
//    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
//                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
//                       NULL);
//
//    swr_init(swrContext);
////    获取通道数  2
//    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
////    反射得到Class类型
//    jclass david_player = env->GetObjectClass(instance);
////    反射得到createAudio方法
//    jmethodID createAudio = env->GetMethodID(david_player, "createTrack", "(II)V");
////    反射调用createAudio
//    env->CallVoidMethod(instance, createAudio, 44100, out_channer_nb);
//    jmethodID audio_write = env->GetMethodID(david_player, "playTrack", "([BI)V");
//
//    int got_frame;
//    while (av_read_frame(pFormatCtx, packet) >= 0) {
//        if (packet->stream_index == audio_stream_idx) {
////            解码  mp3   编码格式frame----pcm   frame
////            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
//            avcodec_send_packet(pCodecCtx, packet);
//            got_frame = avcodec_receive_frame(pCodecCtx, frame);
//            if (!got_frame) {
//                LOGE("解码");
//                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
////                缓冲区的大小
//                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
//                                                      AV_SAMPLE_FMT_S16, 1);
//                jbyteArray audio_sample_array = env->NewByteArray(size);
//                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
//                env->CallVoidMethod(instance, audio_write, audio_sample_array, size);
//                env->DeleteLocalRef(audio_sample_array);
//            }
//        }
//    }
//    av_frame_free(&frame);
//    swr_free(&swrContext);
//    avcodec_close(pCodecCtx);
//    avformat_close_input(&pFormatCtx);
//
//    env->ReleaseStringUTFChars(input_, input);
//}

SLObjectItf engineObject=NULL;//用SLObjectItf声明引擎接口对象
SLEngineItf engineEngine = NULL;//声明具体的引擎对象


SLObjectItf outputMixObject = NULL;//用SLObjectItf创建混音器接口对象
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;////具体的混音器对象实例
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;//默认情况


SLObjectItf audioplayer=NULL;//用SLObjectItf声明播放器接口对象
SLPlayItf  slPlayItf=NULL;//播放器接口
SLAndroidSimpleBufferQueueItf  slBufferQueueItf=NULL;//缓冲区队列接口


size_t buffersize =0;
void *buffer;
//将pcm数据添加到缓冲区中
void getQueueCallBack(SLAndroidSimpleBufferQueueItf  slBufferQueueItf, void* context){

    buffersize=0;

    getPcm(&buffer,&buffersize);
    if(buffer!=NULL&&buffersize!=0){
        //将得到的数据加入到队列中
        (*slBufferQueueItf)->Enqueue(slBufferQueueItf,buffer,buffersize);
    }
}

//创建引擎
void createEngine(){
    slCreateEngine(&engineObject,0,NULL,0,NULL,NULL);//创建引擎
    (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);//实现engineObject接口对象
    (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineEngine);//通过引擎调用接口初始化SLEngineItf
}

//创建混音器
void createMixVolume(){
    (*engineEngine)->CreateOutputMix(engineEngine,&outputMixObject,0,0,0);//用引擎对象创建混音器接口对象
    (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);//实现混音器接口对象
    SLresult   sLresult = (*outputMixObject)->GetInterface(outputMixObject,SL_IID_ENVIRONMENTALREVERB,&outputMixEnvironmentalReverb);//利用混音器实例对象接口初始化具体的混音器对象
    //设置
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverb)->
                SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &settings);
    }
}

//创建播放器
void createPlayer(const char *input) {
    //初始化ffmpeg
    int rate;
    int channels;
    createFFmpeg(&rate, &channels, input);
    LOGE("RATE %d",rate);
    LOGE("channels %d",channels);
    /*
     * typedef struct SLDataLocator_AndroidBufferQueue_ {
    SLuint32    locatorType;//缓冲区队列类型
    SLuint32    numBuffers;//buffer位数
} */

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    /**
    typedef struct SLDataFormat_PCM_ {
        SLuint32 		formatType;  pcm
        SLuint32 		numChannels;  通道数
        SLuint32 		samplesPerSec;  采样率
        SLuint32 		bitsPerSample;  采样位数
        SLuint32 		containerSize;  包含位数
        SLuint32 		channelMask;     立体声
        SLuint32		endianness;    end标志位
    } SLDataFormat_PCM;
     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,channels,rate*1000
            ,SL_PCMSAMPLEFORMAT_FIXED_16
            ,SL_PCMSAMPLEFORMAT_FIXED_16
            ,SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,SL_BYTEORDER_LITTLEENDIAN};

    /*
     * typedef struct SLDataSource_ {
	        void *pLocator;//缓冲区队列
	        void *pFormat;//数据样式,配置信息
        } SLDataSource;
     * */
    SLDataSource dataSource = {&android_queue,&pcm};


    SLDataLocator_OutputMix slDataLocator_outputMix={SL_DATALOCATOR_OUTPUTMIX,outputMixObject};


    SLDataSink slDataSink = {&slDataLocator_outputMix,NULL};


    const SLInterfaceID ids[3]={SL_IID_BUFFERQUEUE,SL_IID_EFFECTSEND,SL_IID_VOLUME};
    const SLboolean req[3]={SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE};

    /*
     * SLresult (*CreateAudioPlayer) (
		SLEngineItf self,
		SLObjectItf * pPlayer,
		SLDataSource *pAudioSrc,//数据设置
		SLDataSink *pAudioSnk,//关联混音器
		SLuint32 numInterfaces,
		const SLInterfaceID * pInterfaceIds,
		const SLboolean * pInterfaceRequired
	);
     * */
    LOGE("执行到此处")
    (*engineEngine)->CreateAudioPlayer(engineEngine,&audioplayer,&dataSource,&slDataSink,3,ids,req);
    (*audioplayer)->Realize(audioplayer,SL_BOOLEAN_FALSE);
    LOGE("执行到此处2")
    (*audioplayer)->GetInterface(audioplayer,SL_IID_PLAY,&slPlayItf);//初始化播放器
    //注册缓冲区,通过缓冲区里面 的数据进行播放
    (*audioplayer)->GetInterface(audioplayer,SL_IID_BUFFERQUEUE,&slBufferQueueItf);
    //设置回调接口
    (*slBufferQueueItf)->RegisterCallback(slBufferQueueItf,getQueueCallBack,NULL);
    //播放
    (*slPlayItf)->SetPlayState(slPlayItf,SL_PLAYSTATE_PLAYING);

    //开始播放
    getQueueCallBack(slBufferQueueItf,NULL);

}
//释放资源
void realseResource(){
    if(audioplayer!=NULL){
        (*audioplayer)->Destroy(audioplayer);
        audioplayer=NULL;
        slBufferQueueItf=NULL;
        slPlayItf=NULL;
    }
    if(outputMixObject!=NULL){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject=NULL;
        outputMixEnvironmentalReverb=NULL;
    }
    if(engineObject!=NULL){
        (*engineObject)->Destroy(engineObject);
        engineObject=NULL;
        engineEngine=NULL;
    }
    realseFFmpeg();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_grechur_ffmpegapp_MusicPlay_play(JNIEnv *env, jobject instance, jstring input_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    createEngine();
    createMixVolume();
    createPlayer(input);

}


extern "C"
JNIEXPORT void JNICALL
Java_com_grechur_ffmpegapp_MusicPlay_stop(JNIEnv *env, jobject instance) {
    realseResource();
}