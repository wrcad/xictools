// Program to print a "die time" months (given as arg) in the future.
// $Id: timegen.cc,v 1.1 2009/04/08 16:47:52 stevew Exp $

#include <stdio.h>
#include <time.h>
#include <stdlib.h>


namespace {
    time_t
    timeset(int months)
    {
        time_t loc;
        time(&loc);
        tm *t = localtime(&loc);
        months += t->tm_mon;
        int years = months/12;
        months %= 12;
        t->tm_year += years;
        t->tm_mon = months;
        return (mktime(t));
    }
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: timegen months\n");
        return (1);
    }

    int m = atoi(argv[1]);
    if (m < 1) {
        fprintf(stderr, "Error: less than one month given.\n");
        return (1);
    }

    time_t t = timeset(m);

    printf("%lu\n", (unsigned long)t);
    return (0);
}

