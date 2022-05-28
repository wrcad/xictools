#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "param.h"

void nrerror(const char *error_text)
{
    fprintf(stderr, "run-time error...");
    fprintf(stderr, "%s\n",error_text);
    exit(1);
}


int read_long_line(FILE *fp, char* line, int *line_no)
{
    if (fgets(line, LONG_LINE_LENGTH, fp) != 0) {
        (*line_no)++;
        return (1);
    }
    return (0);
}


void error_in_line(char* filename, int line_num, char*line)
{
    printf("ERROR IN FILE %s LINE: %d\n%s\n", filename, line_num, line);
    exit(255);
}


void unexpctd_eof(char* filename)
{
    printf("UNEXPECTED END OF FILE: %s\n", filename);
    exit(255);
}


int blank_line(char *line)
{
    int i = 0, n = strlen(line);
    while (i < n) {
        if (!isspace(line[i++]))
            return (0);
    }
    return (1);
}


int skip_arg(char *line, int arg_num)
{
    int i = 0;
    int line_length = strlen(line);
    for(int k = 0; k < arg_num; k++) {
        while (isspace(line[i]) && (i < line_length))
            i++;
        while (!isspace(line[i]) && (i < line_length))
            i++;
    }
    while (isspace(line[i]) && (i < line_length))
        i++;

    return (i);
}


FILE *open_file(char * filename, char *mode)
{
    FILE *fp = fopen(filename, mode);
    if (!fp) {
        fprintf(stderr, "ERROR:\nCannot open file: %s\n\n", filename);
        exit(255);
    }
    return (fp);
}

