
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Interface Routines                                                     *
 *                                                                        *
 *========================================================================*
 $Id: make_key.c,v 2.2 2015/08/29 14:56:28 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * This program generates a random 64 character string in the output file
 * "key.h".
 */

int
main()
{
    FILE *fp;
    int i, j;

    srandom(getpid());
    fp = fopen("key.h", "w");
    if (!fp) {
        printf("Can't open key.h\n");
        return (1);
    }
    fprintf(fp, "static unsigned char key[] = {\n");
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++)
            fprintf(fp, "\'\\x%02x\', ", (int)(random() % 256));
        fprintf(fp, "\n");
    }
    fprintf(fp, "};\n");
    fclose(fp);
    chmod("key.h", 0444);
    return (0);
}

