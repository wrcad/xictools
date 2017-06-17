/*
 * Program to print the date.
 * $Id: datestr.c,v 1.2 2009/04/08 16:47:52 stevew Exp $
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

char *
datestring()

{
    char *tzn;
    struct tm *tp;
    static char tbuf[40];
    char *ap;
    int i;
    struct timeval tv;
    struct timezone tz;

    (void) gettimeofday(&tv, &tz);
    tp = localtime(&tv.tv_sec);
    ap = asctime(tp);
    tzn = (char*)timezone(tz.tz_minuteswest, tp->tm_isdst);
    if (tzn)
        (void) strcat(tbuf, tzn);
    (void) sprintf(tbuf, "%.20s", ap);
    (void) strcat(tbuf, ap + 19);
    i = strlen(tbuf);
    tbuf[i - 1] = '\0';
    return (tbuf);
}

main()
{
    printf("DATE = %s\n", datestring());
}
