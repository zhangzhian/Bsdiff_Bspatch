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

#include "bsdiff.h"

#include <limits.h>
#include <string.h>

#include <sys/types.h>

#include "bzip2/bzlib.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MIN(x, y) (((x)<(y)) ? (x) : (y))

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
 * 保持新文件的大小
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

static int64_t writedata(struct bsdiff_stream *stream, const void *buffer, int64_t length) {
    int64_t result = 0;

    while (length > 0) {
        const int smallsize = (int) MIN(length, INT_MAX);
        const int writeresult = stream->write(stream, buffer, smallsize);
        if (writeresult == -1) {
            return -1;
        }

        result += writeresult;
        length -= smallsize;
        buffer = (uint8_t *) buffer + smallsize;
    }

    return result;
}

struct bsdiff_request {
    const uint8_t *old;
    int64_t oldsize;
    const uint8_t *new;
    int64_t newsize;
    struct bsdiff_stream *stream;
    int64_t *I;
    uint8_t *buffer;
};

static int bsdiff_internal(const struct bsdiff_request req) {
    int64_t *I, *V;
    int64_t scan, pos, len;
    int64_t lastscan, lastpos, lastoffset;
    int64_t oldscore, scsc;
    int64_t s, Sf, lenf, Sb, lenb;
    int64_t overlap, Ss, lens;
    int64_t i;
    uint8_t *buffer;
    uint8_t buf[8 * 3];

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
            len = search(I, req.old, req.oldsize, req.new + scan, req.newsize - scan,
                         0, req.oldsize, &pos);

            for (; scsc < scan + len; scsc++)
                if ((scsc + lastoffset < req.oldsize) &&
                    (req.old[scsc + lastoffset] == req.new[scsc]))
                    oldscore++;

            if (((len == oldscore) && (len != 0)) ||
                (len > oldscore + 8))
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

            /* Write control data */
            if (writedata(req.stream, buf, sizeof(buf)))
                return -1;

            /* Write diff data */
            for (i = 0; i < lenf; i++)
                buffer[i] = req.new[lastscan + i] - req.old[lastpos + i];
            if (writedata(req.stream, buffer, lenf))
                return -1;

            /* Write extra data */
            for (i = 0; i < (scan - lenb) - (lastscan + lenf); i++)
                buffer[i] = req.new[lastscan + lenf + i];
            if (writedata(req.stream, buffer, (scan - lenb) - (lastscan + lenf)))
                return -1;

            lastscan = scan - lenb;
            lastpos = pos - lenb;
            lastoffset = pos - scan;
        };
    };

    return 0;
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
int bsdiff(const uint8_t *oldfile, int64_t oldsize, const uint8_t *newfile, int64_t newsize,
           struct bsdiff_stream *stream) {
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

    result = bsdiff_internal(req);

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


int bsdiff_main(int argc, char *argv[]) {
    //文件句柄
    int fd;
    //bz2错误
    int bz2err;
    //老版本文件 新版本文件
    uint8_t *old, *new;
    //旧版本文件和新版本文件的大小
    off_t oldsize, newsize;
    //大小为8的buff
    uint8_t buf[8];
    //增量文件
    FILE *pf;
    //差分结构体
    struct bsdiff_stream stream;
    //bz2文件
    BZFILE *bz2;

    memset(&bz2, 0, sizeof(bz2));
    stream.malloc = malloc;
    stream.free = free;
    stream.write = bz2_write;

    if (argc != 4) errx(1, "usage: %s oldfile newfile patchfile\n", argv[0]);

    /*
     *  Allocate oldsize+1 bytes instead of oldsize bytes to ensure
     *  that we never try to malloc(0) and get a NULL pointer
     *
     *  旧版本文件分配内存，读出文件内容
     *
     *
     *  int open(const char * pathname, int flags, mode_t mode);
     *
     *  参数 pathname 指向欲打开的文件路径字符串
     *  参数 flags 为文件的打开方式 O_RDONLY 以只读方式打开文件
     *  参数 mode 只有在建立新版本文件时才会生效, 真正建文件时的权限会受到umask值所影响, 因此该文件权限应该为 (mode-umaks).
     *
     *  返回值：成功则返回文件句柄，否则返回-1
     *
     *
     *  off_t lseek(int filedes, off_t offset, int whence);
     *
     *  参数 offset 的含义取决于参数 whence：
     *    1. 如果 whence 是 SEEK_SET，文件偏移量将被设置为 offset。
     *    2. 如果 whence 是 SEEK_CUR，文件偏移量将被设置为 cfo 加上 offset，
     *       offset 可以为正也可以为负。
     *    3. 如果 whence 是 SEEK_END，文件偏移量将被设置为文件长度加上 offset，
     *       offset 可以为正也可以为负。
     *
     *  返回值：新的偏移量（成功），-1（失败）
     *
     *
     *  ssize_t read(int fd, void * buf, size_t count);
     *
     *  参数 void *buf 读上来的数据保存在缓冲区buf中，同时文件的当前读写位置向后移
     *  参数 size_t count 是请求读取的字节数。若参数count 为0, 则read()不会有作用并返回0.
     *
     *  返回值：为实际读取到的字节数
     *
     */
    if (((fd = open(argv[1], O_RDONLY, 0)) < 0) ||
        ((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
        ((old = malloc(oldsize + 1)) == NULL) ||
        (lseek(fd, 0, SEEK_SET) != 0) ||
        (read(fd, old, oldsize) != oldsize) ||
        (close(fd) == -1))
        err(1, "%s", argv[1]);

    /*
     * Allocate newsize+1 bytes instead of newsize bytes to ensure
     * that we never try to malloc(0) and get a NULL pointer
     *
     * 新版本文件分配内存，读取文件内存
     *
     */
    if (((fd = open(argv[2], O_RDONLY, 0)) < 0) ||
        ((newsize = lseek(fd, 0, SEEK_END)) == -1) ||
        ((new = malloc(newsize + 1)) == NULL) ||
        (lseek(fd, 0, SEEK_SET) != 0) ||
        (read(fd, new, newsize) != newsize) ||
        (close(fd) == -1))
        err(1, "%s", argv[2]);

    /*
     * Create the patch file
     *
     * 创建一个patch文件
     */
    if ((pf = fopen(argv[3], "w")) == NULL)
        err(1, "%s", argv[3]);

    /**
     * Write header (signature+newsize)
     *
     * 写头部
     *
     * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
     *
     * 返回值：返回实际写入的数据块数目
     *（1）buffer：是一个指针，对fwrite来说，是要获取数据的地址；
     *（2）size：要写入内容的单字节数；
     *（3）count:要进行写入size字节的数据项的个数；
     *（4）stream:目标文件指针；
     *
     */
    offtout(newsize, buf);
    if (fwrite("ENDSLEY/BSDIFF43", 16, 1, pf) != 1 ||
        fwrite(buf, sizeof(buf), 1, pf) != 1)
        err(1, "Failed to write header");

    /**
     * 以bz2的方式打开增量文件
     */
    if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, pf, 9, 0, 0)))
        errx(1, "BZ2_bzWriteOpen, bz2err=%d", bz2err);

    /**
     * 差分算法
     */
    stream.opaque = bz2;
    if (bsdiff(old, oldsize, new, newsize, &stream))
        err(1, "bsdiff");

    /**
     * 关闭文件
     */
    BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);

    if (bz2err != BZ_OK)
        err(1, "BZ2_bzWriteClose, bz2err=%d", bz2err);

    if (fclose(pf))
        err(1, "fclose");

    /*
     * Free the memory we used
     *
     * 释放使用的内存
     */
    free(old);
    free(new);

    return 0;
}

//#endif
