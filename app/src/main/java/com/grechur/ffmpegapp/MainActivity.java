package com.grechur.ffmpegapp;

import android.Manifest;
import android.os.Build;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    Button play;
    VideoView videoView;
    private MusicPlay musicPlay = new MusicPlay();
    // Used to load the 'native-lib' library on application startup.
//    File inputFile = new File(Environment.getExternalStorageDirectory(), "VID_20180704_131341.mp4");
//    File inputFile = new File(Environment.getExternalStorageDirectory(), "input.wmv");
    File inputFile = new File(Environment.getExternalStorageDirectory(), "input.flv");
    String input = inputFile.getAbsolutePath();
    String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.READ_EXTERNAL_STORAGE};
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions,102);
        }
        play = findViewById(R.id.play);
        videoView = findViewById(R.id.videoView);
        final MusicPlay musicPlay = new MusicPlay();
        Log.e("tag",input.toString());
        play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                videoView.player(input);
                play(input);
            }
        }) ;
    }
    public void play(final String input){
        new Thread(new Runnable() {
            @Override
            public void run() {
                musicPlay.play(input);
            }
        }).start();

    }

    public void stop(View view){
        musicPlay.stop();
    }
}
