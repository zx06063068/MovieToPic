package com.phicomm.decodeutil;

/**
 * Created by xin08.zhang on 2017/10/25.
 */

public class DecodeFrameUtil {
    static {
        System.loadLibrary("decodeframe");
    }
    public static native int decode(String inputurl, String outputurl);
}
