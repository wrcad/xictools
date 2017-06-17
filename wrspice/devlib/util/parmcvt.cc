// usage: parmcvt < infile
//
// This takes a Berlekey-style IFparm definition for device/model
// parameters, and generates the word array and modified struct
// definition used in WRspice.  The stdin is a file containing the
// struct definition (all lines), this produces words.out and
// struct.out for use in the include file.

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// True if p is a leading substring of s.
//
int
prefix(const char *p, const char *s)
{
    if (p == 0 || s == 0)
        return (false);
    while (*p) {
        if (*p != *s)
            return (false);
        p++;
        s++;
    }
    return (true);
}


// Return a copy of the string arg.
//
char *
copy(const char *string)
{
    if (string == 0)
        return (0);
    char *ret, *s;
    s = ret = new char[strlen(string) + 1];
    while ((*s++ = *string++) != 0) ;
    return (ret);
}


main()
{
    char buf[512];
    int cnt = 0;
    if (!fgets(buf, 512, stdin) != 0) {
        fprintf(stderr, "Bad input.\n");
        exit (1);
    }
    FILE *fp1 = fopen("words.out", "w");
    if (!fp1) {
        fprintf(stderr, "Can't open file.\n");
        exit (1);
    }
    FILE *fp2 = fopen("struct.out", "w");
    if (!fp2) {
        fprintf(stderr, "Can't open file.\n");
        exit (1);
    }
    fputs(buf, fp2);
    fputs("static const char *_xxx_[] = {\n", fp1);

    bool in_comment = false;
    char *s;
    while ((s = fgets(buf, 512, stdin)) != 0) {
        while (isspace(*s))
            s++;
        if (s[0] == '/' && s[1] == '*') {
            if (!strstr(s, "*/"))
                in_comment = true;
            continue;
        }
        if (in_comment) {
            if (strstr(s, "*/"))
                in_comment = false;
            continue;
        }

        if (prefix("IOP", s) || prefix("IP", s) || prefix("OP", s)) {
            if (prefix("IOP", s))
                fputs("IO(", fp2);
            else if (prefix("IP", s))
                fputs("IP(", fp2);
            else
                fputs("OP(", fp2);
            char *t = strchr(s, '"');
            if (!t) {
                fprintf(stderr, "Parse error\n");
                exit(1);
            }
            char *e = strchr(t+1, '"');
            if (!e) {
                fprintf(stderr, "Parse error\n");
                exit(1);
            }
            e++;
            char *word = new char[e+2 - t];
            strncpy(word, t, e-t);
            word[e-t] = ',';
            word[e-t + 1] = 0;
            fprintf(fp1, "    %-16s// %d\n", word, cnt);
            while (*e && *e != ',')
                e++;
            if (*e != ',') {
                fprintf(stderr, "Parse error\n");
                exit (1);
            }
            e++;
            while (isspace(*e))
                e++;
            fprintf(fp2, "_xxx_+%d, %s", cnt, e);
            cnt++;
        }
        else if (!*s) {
            fputs("\n", fp1);
            fputs("\n", fp2);
        }
    }
    fputs("};\n", fp1);
    fputs("};\n", fp2);
    fclose(fp1);
    fclose(fp2);
    return (0);
}

