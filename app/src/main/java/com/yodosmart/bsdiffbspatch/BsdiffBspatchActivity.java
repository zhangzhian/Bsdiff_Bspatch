package com.yodosmart.bsdiffbspatch;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.TextView;

import java.util.List;

public class BsdiffBspatchActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bsdiff_bspatch);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);

        String path = Environment.getExternalStorageDirectory().getPath();
//        final String oldpath = path + "/新文件夹/" + "0T01X106508_test.bin" ;
//        final String newpath = path + "/新文件夹/" + "0T01X107508_test.bin";
//        final String patchPath = path + "/新文件夹/" + "test.patch";
//        final String patchNewPath = path + "/新文件夹/" + "test_new.bin";

        //final String newpath = path + "/新文件夹/" + "app.apk";
        final String oldpath = path + "/新文件夹/" + "a.txt" ;
        final String newpath = path + "/新文件夹/" + "b.txt";
        final String patchPath = path + "/新文件夹/" + "c.txt";
        final String patchNewPath = path + "/新文件夹/" + "ipc1_new.apk";

        //测试文件diff
        //int result = DiffPatchUtil.diff(oldpath,newpath,patchPath);
        // 测试文件patch
        //int result = DiffPatchUtil.patch(oldpath,patchNewPath,patchPath);

        /****************************************/
       final String strOld = "select * from db.abc where id = 1";
       final String strNew = "select * from db.efg where id = 9w";
//        String strNew = "select * from db.abc where id = 1";
//        String strOld = "select * from db.efg where id = 99";

//        String strOld = "id = 1 where id = 1 where id = 1";
//        String strNew = "id = 999 where id = 99 where id = 66";

//        String strNew = "id = 1 where id = 1 where id = 1";
//        String strOld = "id = 999 where id = 99 where id = 66";
        //Log.e("zza",strOld.length() + "----" + strNew.length());
        //测试字符串diff
        List<BsdiffBean> result = DiffPatchUtil.diffStr(strOld, strNew, strOld.length(), strNew.length());

        /****************************************/
        //性能测试
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                //int result = DiffPatchUtil.diff(oldpath,newpath,patchPath);
                for (int i = 0; i < 100; i++) {

                    DiffPatchUtil.diffStr(strOld, strNew, strOld.length(), strNew.length());
                    Log.e("zza",i+"");

                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                //int result = DiffPatchUtil.diff(oldpath,newpath,patchPath);
                //int result = DiffPatchUtil.patch(oldpath,patchNewPath,patchPath);

            }
        });
       //thread.start();
       // tv.setText(DiffPatchUtil.diffStr(oldpath, newpath) + "");
    }


}
