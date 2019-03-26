package com.yodosmart.bsdiffbspatch;

import java.util.Arrays;

public class BsdiffBean {

    private byte[] buff;
    private byte[] extra;
    private boolean type;

    public byte[] getBuff() {
        return buff;
    }

    public void setBuff(byte[] buff) {
        this.buff = buff;
    }

    public byte[] getExtra() {
        return extra;
    }

    public void setExtra(byte[] extra) {
        this.extra = extra;
    }

    public boolean isType() {
        return type;
    }

    public void setType(boolean type) {
        this.type = type;
    }

    @Override
    public String toString() {
        return "BsdiffBean{" +
                "buff=" + Arrays.toString(buff) +
                ", extra=" + Arrays.toString(extra) +
                ", type=" + type +
                '}';
    }
}
