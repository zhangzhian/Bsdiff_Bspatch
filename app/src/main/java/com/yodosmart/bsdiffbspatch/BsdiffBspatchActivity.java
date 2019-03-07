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
        final String oldpath = path + "/新文件夹/" + "0T01X106508_test.bin" ;
        final String newpath = path + "/新文件夹/" + "0T01X107508_test.bin";
        final String patchPath = path + "/新文件夹/" + "test.patch";

        final String patchNewPath = path + "/新文件夹/" + "test_new.bin";

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
