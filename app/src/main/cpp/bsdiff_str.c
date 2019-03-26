/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "bsdiff_str.h"


#include <limits.h>
#include <string.h>

#include <sys/types.h>

#include "bzip2/bzlib.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include<android/log.h>

#define TAG "zza-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型


#define MIN(x, y) (((x)<(y)) ? (x) : (y))

struct bsdiff_stream
{
    void* opaque;

    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
};

struct bsdiff_request {
    const uint8_t *old;
    int64_t oldsize;
    const uint8_t *new;
    int64_t newsize;
    struct bsdiff_stream *stream;
    int64_t *I;
    uint8_t *buffer;
};

static void split(int64_t *I, int64_t *V, int64_t start, int64_t len, int64_t h) {
    int64_t i, j, k, x, tmp, jj, kk;

    if (len < 16) {
        for (k = start; k < start + len; k += j) {
            j = 1;
            x = V[I[k] + h];
            for (i = 1; k + i < start + len; i++) {
                if (V[I[k + i] + h] < x) {
                    x = V[I[k + i] + h];
                    j = 0;
                };
                if (V[I[k + i] + h] == x) {
                    tmp = I[k + j];
                    I[k + j] = I[k + i];
                    I[k + i] = tmp;
                    j++;
                };
            };
            for (i = 0; i < j; i++) V[I[k + i]] = k + j - 1;
            if (j == 1) I[k] = -1;
        };
        return;
    };

    x = V[I[start + len / 2] + h];
    jj = 0;
    kk = 0;
    for (i = start; i < start + len; i++) {
        if (V[I[i] + h] < x) jj++;
        if (V[I[i] + h] == x) kk++;
    };
    jj += start;
    kk += jj;

    i = start;
    j = 0;
    k = 0;
    while (i < jj) {
        if (V[I[i] + h] < x) {
            i++;
        } else if (V[I[i] + h] == x) {
            tmp = I[i];
            I[i] = I[jj + j];
            I[jj + j] = tmp;
            j++;
        } else {
            tmp = I[i];
            I[i] = I[kk + k];
            I[kk + k] = tmp;
            k++;
        };
    };

    while (jj + j < kk) {
        if (V[I[jj + j] + h] == x) {
            j++;
        } else {
            tmp = I[jj + j];
            I[jj + j] = I[kk + k];
            I[kk + k] = tmp;
            k++;
        };
    };

    if (jj > start) split(I, V, start, jj - start, h);

    for (i = 0; i < kk - jj; i++) V[I[jj + i]] = kk - 1;
    if (jj == kk - 1) I[jj] = -1;

    if (start + len > kk) split(I, V, kk, start + len - kk, h);
}

static void qsufsort(int64_t *I, int64_t *V, const uint8_t *old, int64_t oldsize) {
    int64_t buckets[256];
    int64_t i, h, len;

    for (i = 0; i < 256; i++) buckets[i] = 0;
    for (i = 0; i < oldsize; i++) buckets[old[i]]++;
    for (i = 1; i < 256; i++) buckets[i] += buckets[i - 1];
    for (i = 255; i > 0; i--) buckets[i] = buckets[i - 1];
    buckets[0] = 0;

    for (i = 0; i < oldsize; i++) I[++buckets[old[i]]] = i;
    I[0] = oldsize;
    for (i = 0; i < oldsize; i++) V[i] = buckets[old[i]];
    V[oldsize] = 0;
    for (i = 1; i < 256; i++) if (buckets[i] == buckets[i - 1] + 1) I[buckets[i]] = -1;
    I[0] = -1;

    for (h = 1; I[0] != -(oldsize + 1); h += h) {
        len = 0;
        for (i = 0; i < oldsize + 1;) {
            if (I[i] < 0) {
                len -= I[i];
                i -= I[i];
            } else {
                if (len) I[i - len] = -len;
                len = V[I[i]] + 1 - i;
                split(I, V, i, len, h);
                i += len;
                len = 0;
            };
        };
        if (len) I[i - len] = -len;
    };

    for (i = 0; i < oldsize + 1; i++) I[V[i]] = i;
}

static int64_t matchlen(const uint8_t *old, int64_t oldsize, const uint8_t *new, int64_t newsize) {
    int64_t i;

    for (i = 0; (i < oldsize) && (i < newsize); i++)
        if (old[i] != new[i]) break;

    return i;
}

static int64_t search(const int64_t *I, const uint8_t *old, int64_t oldsize,
                      const uint8_t *new, int64_t newsize, int64_t st, int64_t en, int64_t *pos) {
    int64_t x, y;

    if (en - st < 2) {
        x = matchlen(old + I[st], oldsize - I[st], new, newsize);
        y = matchlen(old + I[en], oldsize - I[en], new, newsize);

        if (x > y) {
            *pos = I[st];
            return x;
        } else {
            *pos = I[en];
            return y;
        }
    };

    x = st + (en - st) / 2;
    if (memcmp(old + I[x], new, MIN(oldsize - I[x], newsize)) < 0) {
        return search(I, old, oldsize, new, newsize, x, en, pos);
    } else {
        return search(I, old, oldsize, new, newsize, st, x, pos);
    };
}

/**
 * 保存新文件的大小
 * @param x  文件的大小
 * @param buf buff
 */
static void offtout(int64_t x, uint8_t *buf) {
    int64_t y;

    if (x < 0) y = -x; else y = x;

    buf[0] = y % 256;
    y -= buf[0];
    y = y / 256;
    buf[1] = y % 256;
    y -= buf[1];
    y = y / 256;
    buf[2] = y % 256;
    y -= buf[2];
    y = y / 256;
    buf[3] = y % 256;
    y -= buf[3];
    y = y / 256;
    buf[4] = y % 256;
    y -= buf[4];
    y = y / 256;
    buf[5] = y % 256;
    y -= buf[5];
    y = y / 256;
    buf[6] = y % 256;
    y -= buf[6];
    y = y / 256;
    buf[7] = y % 256;

    if (x < 0) buf[7] |= 0x80;
}

static int bsdiff_internal(const struct bsdiff_request req,struct bsdiff_result *result_data) {
    int64_t *I, *V;
    int64_t scan, pos, len;
    int64_t lastscan, lastpos, lastoffset;
    int64_t oldscore, scsc;
    int64_t s, Sf, lenf, Sb, lenb;
    int64_t overlap, Ss, lens;
    int64_t i;
    uint8_t *buffer;
    uint8_t buf[8 * 3];
    int8_t num = 0;
    if ((V = req.stream->malloc((req.oldsize + 1) * sizeof(int64_t))) == NULL) return -1;
    I = req.I;

    qsufsort(I, V, req.old, req.oldsize);
    req.stream->free(V);

    buffer = req.buffer;

    /* Compute the differences, writing ctrl as we go */
    scan = 0;
    len = 0;
    pos = 0;
    lastscan = 0;
    lastpos = 0;
    lastoffset = 0;
    while (scan < req.newsize) {
        oldscore = 0;

        for (scsc = scan += len; scan < req.newsize; scan++) {

            len = search(I, req.old, req.oldsize,
                    req.new + scan, req.newsize - scan, 0, req.oldsize, &pos);

            for (; scsc < scan + len; scsc++)
                if ((scsc + lastoffset < req.oldsize) &&
                    (req.old[scsc + lastoffset] == req.new[scsc]))
                    oldscore++;
            //TODO 原本为8
            if (((len == oldscore) && (len != 0)) ||
                (len > oldscore + 4))
                break;

            if ((scan + lastoffset < req.oldsize) &&
                (req.old[scan + lastoffset] == req.new[scan]))
                oldscore--;
        };

        if ((len != oldscore) || (scan == req.newsize)) {
            s = 0;
            Sf = 0;
            lenf = 0;
            for (i = 0; (lastscan + i < scan) && (lastpos + i < req.oldsize);) {
                if (req.old[lastpos + i] == req.new[lastscan + i]) s++;
                i++;
                if (s * 2 - i > Sf * 2 - lenf) {
                    Sf = s;
                    lenf = i;
                };
            };

            lenb = 0;
            if (scan < req.newsize) {
                s = 0;
                Sb = 0;
                for (i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
                    if (req.old[pos - i] == req.new[scan - i]) s++;
                    if (s * 2 - i > Sb * 2 - lenb) {
                        Sb = s;
                        lenb = i;
                    };
                };
            };

            if (lastscan + lenf > scan - lenb) {
                overlap = (lastscan + lenf) - (scan - lenb);
                s = 0;
                Ss = 0;
                lens = 0;
                for (i = 0; i < overlap; i++) {
                    if (req.new[lastscan + lenf - overlap + i] ==
                        req.old[lastpos + lenf - overlap + i])
                        s++;
                    if (req.new[scan - lenb + i] ==
                        req.old[pos - lenb + i])
                        s--;
                    if (s > Ss) {
                        Ss = s;
                        lens = i + 1;
                    };
                };

                lenf += lens - overlap;
                lenb -= lens;
            };

            offtout(lenf, buf);
            offtout((scan - lenb) - (lastscan + lenf), buf + 8);
            offtout((pos - lenb) - (lastpos + lenf), buf + 16);

            LOGE("--------------------------");

            /* Write diff data */
            for (i = 0; i < lenf; i++){
                buffer[i] = req.new[lastscan + i] - req.old[lastpos + i];
                result_data[num].buffer[i] = buffer[i];
                //LOGE("%d",buffer[i]);
                //LOGE("lastscan: %d lastpos: %d",lastscan + i,lastpos + i);
            }

            result_data[num].buffsize = lenf;

            //LOGE("Write diff data: %s ",buffer);
            LOGE("Write diff data: %d ",lenf);

            /* Write extra data */
            for (i = 0; i < (scan - lenb) - (lastscan + lenf); i++) {
                buffer[i] = req.new[lastscan + lenf + i];
                result_data[num].extra[i] = buffer[i];
            }

            result_data[num].extrasize = (scan - lenb) - (lastscan + lenf);
            LOGE("Write extra data: %s ",buffer);
            LOGE("Write extra data: %d ",(scan - lenb) - (lastscan + lenf));


            //新文件扫描的位置
            lastscan = scan - lenb;
            lastpos = pos - lenb;
            lastoffset = pos - scan;

            num++;
            //LOGE("lastscan data: %d ", lastscan);
            //LOGE("lastpos data: %d ", lastpos);
            //LOGE("lastoffset data: %d ", lastoffset);

        };
    };
    LOGE("================================");
    return num;
}

/**
 * 差分算法
 * @param oldfile 老版本文件
 * @param oldsize 老版本大小
 * @param newfile 新版本文件
 * @param newsize 新版本大小
 * @param stream  差分结构体
 * @return 
 */
int bsdiff_c(const uint8_t *oldfile, int64_t oldsize, const uint8_t *newfile, int64_t newsize,
           struct bsdiff_stream *stream, struct bsdiff_result *result_data) {
    int result;
    struct bsdiff_request req;

    if ((req.I = stream->malloc((oldsize + 1) * sizeof(int64_t))) == NULL)
        return -1;

    if ((req.buffer = stream->malloc(newsize + 1)) == NULL) {
        stream->free(req.I);
        return -1;
    }

    req.old = oldfile;
    req.oldsize = oldsize;
    req.new = newfile;
    req.newsize = newsize;
    req.stream = stream;

    result = bsdiff_internal(req, result_data);

    stream->free(req.buffer);
    stream->free(req.I);

    return result;
}

//#if defined(BSDIFF_EXECUTABLE)



static int bz2_write(struct bsdiff_stream *stream, const void *buffer, int size) {
    int bz2err;
    BZFILE *bz2;

    bz2 = (BZFILE *) stream->opaque;
    BZ2_bzWrite(&bz2err, bz2, (void *) buffer, size);
    if (bz2err != BZ_STREAM_END && bz2err != BZ_OK)
        return -1;

    return 0;
}


int bsdiff_str(char *oldStr, char *newStr, int oldSize, int newSize, struct  bsdiff_result *result_new, struct  bsdiff_result *result_old) {

    //老版本文件 新版本文件
    uint8_t *old, *new;
    //旧版本文件和新版本文件的大小
    off_t oldsize, newsize;
    //大小为8的buff
    uint8_t buf[8];

    //差分结构体
    struct bsdiff_stream stream;

    stream.malloc = malloc;
    stream.free = free;

    newsize = newSize;
    oldsize = oldSize;
    old = oldStr;
    new = newStr;

   int result_new_num = bsdiff_c(old, oldsize, new, newsize, &stream, result_new);

   int result_old_num = bsdiff_c(new, newsize, old, oldsize, &stream, result_old);

   if(result_new_num == result_old_num){
       return result_new_num;
   }

   return 0;
}


//#endif
