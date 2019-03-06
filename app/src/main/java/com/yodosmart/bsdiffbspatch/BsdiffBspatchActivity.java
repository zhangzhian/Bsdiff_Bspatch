package com.yodosmart.bsdiffbspatch;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.TextView;

public class BsdiffBspatchActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bsdiff_bspatch);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);

        String path = Environment.getExternalStorageDirectory().getPath();
        final String oldpath = path + "/新文件夹/" + "ipc021501.apk" ;
        final String newpath = path + "/新文件夹/" + "ipc030601.apk";
        final String patchPath = path + "/新文件夹/" + "ipc.patch";

        final String patchNewPath = path + "/新文件夹/" + "ipc030602.apk";
//        File oldFile = new File(oldpath);
//        File newFile = new File(newpath);
//        File patchFile = new File(patchPath);

//        Log.e("zza","oldFile:"+oldFile.exists());
//        Log.e("zza","newFile:"+newFile.exists());
//        Log.e("zza","patchFile:"+patchFile.exists());


        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                //int result = DiffPatchUtil.diff(oldpath,newpath,patchPath);
                int result = DiffPatchUtil.patch(oldpath,patchNewPath,patchPath);
                Log.e("zza",result+"");
            }
        });
        thread.start();
        //tv.setText(DiffPatchUtil.printPath(oldpath,newpath,patchPath)+"");
    }


}
