package com.phicomm.mydecodefram;

import android.os.Environment;
import android.os.StatFs;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.phicomm.decodeutil.DecodeFrameUtil;

import java.io.File;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private Button codec;
    private TextView tv_info;
    private Button btnDecoder;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init();
    }

    private void init() {

        codec = (Button) findViewById(R.id.btn_codec);
        tv_info= (TextView) findViewById(R.id.tv_info);
        btnDecoder= (Button) findViewById(R.id.button);
        codec.setOnClickListener(this);
        btnDecoder.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {

            case R.id.btn_codec:
                //tv_info.setText(avcodecinfo());
                Log.d("pathtest", "getDataDirectory" + android.os.Environment.getDataDirectory().getPath());
                Log.d("pathtest", "getRootDirectory" + android.os.Environment.getRootDirectory().getPath());
                Log.d("pathtest", "internal path" + android.os.Environment.getExternalStorageDirectory().getPath());
                Log.d("pathtest","DataDirectory avil:"+getRomAvalibleSize(android.os.Environment.getDataDirectory().getPath()));
                Log.d("pathtest","RootDirectory avil:"+getRomAvalibleSize(android.os.Environment.getRootDirectory().getPath()));
                Log.d("pathtest","ExternalStorageDirectory avil:"+getRomAvalibleSize(android.os.Environment.getExternalStorageDirectory().getPath()+"/Android"));
                break;

            case R.id.button:
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        startDecode();
                    }
                }).start();
                break;

            default:
                break;
        }
    }

    private void startDecode() {
        File file = new File("/storage/emulated/0/testvideo/mkv.mkv");
        String inputurl = file.getAbsolutePath();
        String outputurl = file.getParent() + "/mkvtmp/";
        File destfile = new File(outputurl);
        if (!destfile.exists() || !destfile.isDirectory()) {
            destfile.mkdirs();
        }
        outputurl = destfile.getPath() + "/FactoryTest";
        Log.e("ws-----------inputurl", inputurl);
        Log.e("ws------------outputurl", outputurl);
        Log.d("Decode","frame count:"+DecodeFrameUtil.decode(inputurl, outputurl));
    }

    public static String getRomAvalibleSize(String path) {
        StatFs mStatFs = new StatFs(path);
        long mBlockSize = mStatFs.getBlockSize();
        long mAvailbaleCount = mStatFs.getAvailableBlocks();
        return String.format(Locale.ENGLISH, "%.2f", mAvailbaleCount * mBlockSize / 1024.0 / 1024.0 / 1024.0) + "G";
    }

}

