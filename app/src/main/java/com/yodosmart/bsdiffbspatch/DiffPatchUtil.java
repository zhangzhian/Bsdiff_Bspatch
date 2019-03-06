package com.yodosmart.bsdiffbspatch;

/**
 * @Author: 张志安
 * @Mail: zhangzhian2016@gmail.com
 * @Date: 2019/3/6 15:44
 */
public class DiffPatchUtil {

    static {
        System.loadLibrary("diff-patch-lib");
    }

    /**
     * native方法 比较路径为oldPath的文件与newPath的文件之间差异，并生成patch包，存储于patchPath
     *
     * 返回：0，说明操作成功
     *
     * @param oldPath 示例:/sdcard/old.apk
     * @param newPath 示例:/sdcard/new.apk
     * @param patchPath  示例:/sdcard/xx.patch
     * @return
     */
    public static native int diff(String oldPath, String newPath, String patchPath);


    /**
     * native方法 使用路径为oldPath的文件与路径为patchPath的补丁包，合成新的文件，并存储于newPath
     *
     * 返回：0，说明操作成功
     *
     * @param oldPath 示例:/sdcard/old.apk
     * @param newPath 示例:/sdcard/new.apk
     * @param patchPath  示例:/sdcard/xx.patch
     * @return
     */
    public static native int patch(String oldPath, String newPath,
                                   String patchPath);

    public static native int printPath(String oldPath, String newPath,
                                   String patchPath);
}
