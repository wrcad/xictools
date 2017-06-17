
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

/* $Id: utod.c,v 1.3 2014/03/01 01:42:24 stevew Exp $ */
/* change unix format text files to DOS format */

const char *bins[] = { "exe", "dll", "zip", "gz", "dat", NULL };

int main(int argc, char **argv)
{
    int i;
    FILE *ip, *op;
    char tname[32], *tf;

    strcpy(tname, "duXXXXXX");
    tf = mktemp(tname);

    for (i = 1; i < argc; i++) {
        struct stat st;
        const char *e = strrchr(argv[i], '.');
        if (e != NULL) {
            e++;
            const char **p;
            for (p = bins; *p != NULL; p++) {
                if (!strcasecmp(*p, e))
                    break;
            }
            if (*p != NULL)
                continue;
        }
        if (stat(argv[i], &st) < 0) {
            fprintf(stderr, "can't stat: %s\n",argv[i]);
            continue;
        }
        if (!(st.st_mode & S_IFREG))
            continue;

        ip = fopen(argv[i], "rb");
        if (ip == NULL) {
            fprintf(stderr, "can't open: %s\n",argv[i]);
            continue;
        }
        op = fopen(tf, "wb");
        if (op == NULL) {
            fprintf(stderr, "internal error: can't open temp file\n");
            exit(1);
        }

        {
            int c, lc = 0;
            while ((c = getc(ip)) != EOF) {
                if (c == 10 && lc != 13)
                    putc(13, op);
                putc(c, op);
                lc = c;
            }
        }
        fclose(ip);
        fclose(op);

        op = fopen(argv[i], "wb");
        if (op == NULL) {
            fprintf(stderr, "can't open: %s\n",argv[i]);
            continue;
        }
        ip = fopen(tf, "rb");
        if (ip == NULL) {
            fprintf(stderr, "internal error: can't open temp file\n");
            exit(1);
        }

        {
            int c;
            while ((c = getc(ip)) != EOF) {
                putc(c, op);
            }
        }
        fclose(ip);
        fclose(op);
    }
    unlink(tf);
    return (0);
}

