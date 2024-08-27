/* testmini.c -- very simple test program for the miniLZO library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>


/*************************************************************************
// This program shows the basic usage of the LZO library.
// We will compress a block of data and decompress again.
//
// For more information, documentation, example programs and other support
// files (like Makefiles and build scripts) please download the full LZO
// package from
//    http://www.oberhumer.com/opensource/lzo/
**************************************************************************/

/* First let's include "minizo.h". */

#include "minilzo.h"


/* We want to compress the data block at 'in' with length 'IN_LEN' to
 * the block at 'out'. Because the input block may be incompressible,
 * we must provide a little more output space in case that compression
 * is not possible.
 */

#define IN_LEN      (64*1024ul)
#define OUT_LEN     (IN_LEN * 2 + IN_LEN / 16 + 64 + 3)

static unsigned char in[IN_LEN];
static unsigned char out[OUT_LEN];


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#if 0
uint32_t wrkmem [((LZO1X_1_MEM_COMPRESS) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ];

#else
#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);
#endif /* 0 */

#define errx err
void err(int exitcode, const char *fmt, ...)
{
    va_list valist;
    va_start(valist, fmt);
    vprintf(fmt, valist);
    va_end(valist);
    exit(exitcode);
}

#define sigbit(val) ((sizeof(val) << 3) -1)

#if defined (sigbit)
#define GetBit(val)  (unsigned char)(((val)>>(sigbit(val))) & 0x1)
#define SetBit(val)  val= (lzo_uint) ((val) | (0x1 << (sigbit(val))))
#define ResetBit(val) val= (lzo_uint) ((val) & ~(0x1 << (sigbit(val))))
#else
#define GetBit(val,bit)  (unsigned char)(((val)>>(bit)) & 0x1)
#define SetBit(val,bit)  val= (lzo_uint) ((val) | (0x1 << (bit)))
#define ResetBit(val,bit) val= (lzo_uint) ((val) & ~(0x1 << (bit)))
#endif


int main(int argc, char *argv[])
{
    FILE *fs, *fp;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint new_len;
    uint8_t cflag = 0;
    uint32_t i;
    uint32_t cnt = 0;

    if(argc != 3 && argv == NULL)
        errx(1, "usage: infile outfile\n");

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");

    if(lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        return 3;
    }

    if((fs = fopen(argv[1], "rb")) == NULL) errx(1, "Open failed :%s", fs);
    if((fp = fopen(argv[2], "wb+")) == NULL) errx(1, "Open failed (%s)", fp);

    /* compress from 'in' to 'out' with LZO1X-1 */
    cnt = 0;
    while(feof(fs) == 0)
    {
        if((in_len = fread(in, 1, IN_LEN, fs)))
        {
            i = lzo1x_1_compress(in, in_len, out, &out_len, wrkmem);

            if(i == LZO_E_OK)
            {
                if(out_len < in_len)
                {
                    if(fwrite(&out_len, sizeof(out_len), 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);
                    if(fwrite(out, out_len, 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);
                }
                else
                {
                    out_len = in_len;
                    SetBit(out_len);

                    if(fwrite(&out_len, sizeof(out_len), 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);
                    if(fwrite(in, in_len, 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);
                    
                    printf("This block contains incompressible data.\n");
                }

                printf("%d compressed %lu bytes into %lu bytes\n",
                       ++cnt, in_len, ResetBit(out_len));
            }
            else
            {
                /* this should NEVER happen */
                printf("internal error - compression failed: %d\n", i);
                return 2;
            }
        }
    }

    if(fclose(fs) == -1)	errx(1, "Close failed :%s", fs);
    if(fclose(fp) == -1)	errx(1, "Close failed :%s", fp);

    /* decompress again, now going from 'out' to 'in'  */
    if((fs = fopen(argv[2], "rb")) == NULL) errx(1, "Open failed :%s", fs);
    if((fp = fopen("de_test", "wb+")) == NULL) errx(1, "Open failed (%s)", fp);

    cnt = 0;
    while(fread(&new_len, 1, sizeof(new_len), fs))
    {
        cflag = GetBit(new_len);
        
        if(cflag)
        {
            ResetBit(new_len);
        }
        
        if((in_len = fread(in, 1, new_len, fs)) == 0)
        {
            printf("error!!!\n");
            break;
        }
        
        if(cflag == 0)
        {
            i = lzo1x_decompress(in, in_len, out, &out_len, NULL);

            if(i == LZO_E_OK)
            {
                if(fwrite(out, out_len, 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);

                printf("%d decompressed %lu bytes back into %lu bytes\n",
                       ++cnt, (unsigned long) out_len, (unsigned long) in_len);
            }
            else
            {
                /* this should NEVER happen */
                printf("internal error - decompression failed: %d\n", i);
                return 1;
            }
        }
        else
        {
            if(fwrite(in, new_len, 1, fp) != 1) errx(1, "fwrite failed (%s)", fp);
        
            printf("%d decompressed %lu bytes back into %lu bytes\n",
                   ++cnt, (unsigned long) new_len, (unsigned long) new_len);
        }
    }

    if(fclose(fs) == -1)	errx(1, "Close failed :%s", fs);
    if(fclose(fp) == -1)	errx(1, "Close failed :%s", fp);

    printf("\nminiLZO simple compression test passed.\n");
    return 0;
}


/* vim:set ts=4 sw=4 et: */
