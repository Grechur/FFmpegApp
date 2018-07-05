package com.grechur.ffmpegapp;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by zz on 2018/7/5.
 */

public class VideoView extends SurfaceView{
    private SurfaceHolder holder;
    static {
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("native-lib");
    }
    public VideoView(Context context) {
        this(context,null);
    }

    public VideoView(Context context, AttributeSet attrs) {
        this(context, attrs,0);
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        holder = this.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }
    public void player(final String input) {
        new Thread(new Runnable(){
                   @Override
                   public void run() {
                       //        绘制功能 不需要交给SurfaveView        VideoView.this.getHolder().getSurface()
                       render(input, VideoView.this.getHolder().getSurface());
                   }
               }
        ).start();
    }
    public static native void render(String input,Surface surface);
}
