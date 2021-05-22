/*
 * uri.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef DEFAULT_BUFSIZE
#define DEFAULT_BUFSIZE 1024
/* DEFAULT_BUFSIZE */
#endif

int encode_uri(char *src, char *dest)
{
    int     status  = 0;

    size_t  bufsiz  = DEFAULT_BUFSIZE,
            current = 0;    /* dest のオフセット */

    char*   p       = src;  /* src をなめる */

    /* ヌル文字 '\0' が出現するまでぐるぐる回す */
    while (*p != '\0') {
        /*
         * true:  アドレス p が指す領域の値は変換対象
         * false: アドレス p が指す領域の値は変換対象外
         */
        if (
                (*p >= 0x20 && *p <= 0x39)  ||  /* [0-9] */
                (*p >= 0x41 && *p <= 0x5A)  ||  /* [A-Z] */
                (*p >= 0x61 && *p <= 0x7A)  ||  /* [a-z] */
                (*p == 0x2E)                ||  /* . */
                (*p == 0x2F)                ||  /* / */
                (*p == 0x3A)                    /* : */
           ) {
            /* アドレス p が指す領域の値をそのまま代入 */
            *(dest + current) = *p;
            /* dest のオフセットを加算 */
            current++;
        } else {
            /*
             * アドレス p が指す領域の値を、二桁の16進数へ変換
             * 途中、暗黙のキャストにより 4byte へ拡張され出力されてしまうので、
             * ビットへのAND操作により、下位 8bit (1byte) ぶんを取り出す
             */
            current += sprintf(dest + current, "%%%02X",
			       *p & 0x000000FF);
        }
        /* アドレスを加算 */
        p++;
    }

    return 0;

ERR:
    /* エラー処理 */
    switch (status) {
        case    -1:
        case    -2:
            fprintf(stderr, "%s\n",
                    strerror(errno));
            break;
    }

    return status;
}

int decode_uri(char *src, char *dest)
{
    int     status  = 0;

    size_t  bufsiz  = DEFAULT_BUFSIZE,
            current = 0;    /* dest のオフセット */

    char*   p       = src;  /* src を舐める */

    /* ヌル文字 '\0' が出現するまでぐるぐる回す */
    while (*p != '\0') {
        /*
         * true:  *p == '%' なので変換対象
         * false: その他は変換対象外
         */
        if (*p == '%') {
            /* アドレスを加算 (%を飛ばす) */
            p++;
            /* 二桁 (文字なので2byte) の16進数をデコードし、 dest + current へ代入 */
            sscanf(p, "%2X", dest + current);
            /* アドレスを加算 (XXを飛ばす) */
            p += 2;
	} else if (*p == '_') {
            *(dest + current) = ' ';
            p++;
        } else {
            /* アドレス p が指す領域の値をそのまま代入 */
            *(dest + current) = *p;
            /* アドレスを加算 */
            p++;
        }
        /* dest のオフセットを加算 */
        current++;
    }

    return 0;

ERR:
    /* エラー処理 */
    switch (status) {
        case    -1:
        case    -2:
            fprintf(stderr, "%s\n",
                    strerror(errno));
            break;
    }

    return status;
}
