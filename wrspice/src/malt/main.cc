#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "qnrutil.h"
#include "param.h"
#include "yield.h"
#include "optimizer.h"


int main(int argc, const char *argv[])
{
    if (!param.opt_begin(argc, argv))
        exit(255);
    OPTIMIZER opt;
    opt.opt_begin();
    return (0);
}

