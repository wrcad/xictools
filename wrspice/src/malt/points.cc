#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "miscutil/lstring.h"
#include "param.h"
#include "qnrutil.h"
#include "optimizer.h"

#ifdef WIN32
#define BOUNDARY_CALL       " boundary > NUL"
#else
#define BOUNDARY_CALL       " boundary > trash"
#endif


namespace {
    bool read_long_line(FILE *fp, char* line, int *line_no)
    {
        if (fgets(line, LONG_LINE_LENGTH, fp) != 0) {
            (*line_no)++;
            return (true);
        }
        return (false);
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

    bool blank_line(const char *line)
    {
        for (const char *c = line; *c; c++) {
            if (!isspace(*c))
                return (false);
        }
        return (true);
    }

    bool skip_comments(FILE *fp, char *buf, int *linecnt)
    {
        do {
            if (!read_long_line(fp, buf, linecnt))
                return (false);
        } while(buf[0] == '*' || blank_line(buf));
        return (true);
    }
}


void
OPTIMIZER::readinit()
{
    char in_buffer[LONG_LINE_LENGTH];
    char errmssg[LINE_LENGTH+30];

    // String out the input file name.
    char filename[LINE_LENGTH];
    strcpy(filename, param.input_dir());
    strcat(filename, param.circuit_name());
    strcat(filename, param.init_ext());
    strcat(filename, param.opt_num());

    // String out the error message.
    strcpy(errmssg, "Can't read the '");
    strcat(errmssg, filename);
    strcat(errmssg, "'file.\n");

    // Read init file.
    FILE *fp = fopen(filename, "r");
    if (!fp)
        nrerror(errmssg);

    int line_num = 0;
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    // Initial allocations performed here.
    int d;
    if (sscanf(in_buffer, "%d", &d) != 1 || !set_dim(d))
        error_in_line(filename, line_num, in_buffer);

    // Read some more...

    // Parameter names.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    const char *cp = in_buffer;
    for (int x = 0; x < o_dim; x++) {
        char *n = lstring::gettok(&cp);
        if (!set_param_name(x, n))
            error_in_line(filename, line_num, in_buffer);
        delete [] n;
    }       

    // Nominal values.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        double v;
        if (sscanf(tok, "%lf", &v) != 1 || !set_nominal(x, v))
            error_in_line(filename, line_num, in_buffer);
    }

    // Minimum parameter search values.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        double v;
        if (sscanf(tok, "%lf", &v) != 1 || !set_lower(x, v))
            error_in_line(filename, line_num, in_buffer);
    }

    // Maximum parameter search values.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        double v;
        if (sscanf(tok, "%lf", &v) != 1 || !set_upper(x, v))
            error_in_line(filename, line_num, in_buffer);
    }

    // Percent variations.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        double v;
        if (sscanf(tok, "%lf", &v) != 1 || !set_scale(x, v))
            error_in_line(filename, line_num, in_buffer);
    }

    // Yield.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        if (sscanf(tok, "%d", &d) != 1 || !set_yield(x, d))
            error_in_line(filename, line_num, in_buffer);
    }

    // Static.
    if (!skip_comments(fp, in_buffer, &line_num))
        unexpctd_eof(filename);
    cp = in_buffer;
    for (int x = 1; x <= o_dim; x++) {
        char *tok = lstring::gettok(&cp);
        if (sscanf(tok, "%d", &d) != 1 || !set_statics(x, d))
            error_in_line(filename, line_num, in_buffer);
    }

    // Finalize.
    finalize_setup();

    fclose(fp);
}


#define MAX_DIMS 10

bool
OPTIMIZER::set_dim(int d)
{
    if (d < 1 || d > MAX_DIMS)
        return (false);
    o_dim = d;

    // Initialize.
    o_startpnt  = vector(1, o_dim);
    o_centerpnt = vector(1, o_dim);
    o_lower     = vector(1, o_dim);
    o_upper     = vector(1, o_dim);
    o_scale     = vector(1, o_dim);
    o_yield     = ivector(1, o_dim);
    o_statics   = ivector(1, o_dim);
    o_names     = new const char*[o_dim];
    memset(o_names, 0, o_dim*sizeof(const char*));
    o_delta     = new double[o_dim+1];
    memset(o_delta, 0, (o_dim+1)*sizeof(double));

    return (true);
}


bool
OPTIMIZER::set_param_name(int ix, const char *n)
{
    if (ix < 0 || ix >= MAX_DIMS || !n)
        return (false);
    o_names[ix] = lstring::copy(n);
    return (true);
}


bool
OPTIMIZER::set_nominal(int ix, double v)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_centerpnt[ix] = v;
    return (true);
}


bool
OPTIMIZER::set_lower(int ix, double v)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_lower[ix] = v;
    return (true);
}


bool
OPTIMIZER::set_upper(int ix, double v)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_upper[ix] = v;
    return (true);
}


bool
OPTIMIZER::set_scale(int ix, double v)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_scale[ix] = v;
    return (true);
}


bool
OPTIMIZER::set_yield(int ix, int y)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_yield[ix] = y;
    if (y)
        o_var++;
    return (true);
}


bool
OPTIMIZER::set_statics(int ix, int y)
{
    if (ix < 1 || ix > MAX_DIMS)
        return (false);
    o_statics[ix] = y;
    if (y)
        o_pin++;
    return (true);
}


void
OPTIMIZER::finalize_setup()
{

    o_xyz = ivector(1, o_pin);
    o_pin = 0;
    for (int x = 1; x <= o_dim; x++) {
        if (o_statics[x])
            o_xyz[++o_pin] = x;
    }
    
    // Scale it.
    for (int x = 1; x <= o_dim; x++) {
        o_scale[x] *= fabs(o_centerpnt[x]);
        o_centerpnt[x] /= o_scale[x];
        o_lower[x] /= o_scale[x];
        o_upper[x] /= o_scale[x];
        o_startpnt[x] = o_centerpnt[x];
    }
}


int
OPTIMIZER::addpoint()
{
    char errmsg[LINE_LENGTH];
    char system_call[LINE_LENGTH];

    FILE *fp = fopen(LIMITS_FILE,"w");
    if (!fp) {
        strcpy(errmsg, "Can't write to the '");
        strcat(errmsg, LIMITS_FILE);
        strcat(errmsg, "' file.\n");
        nrerror(errmsg);
    }

    // Load zeroeth element with unscaled value for accuracy.
    double dist = 0.0;
    for (int x = 1; x <= o_dim; x++)
        dist += (o_pc[x]-o_po[x])*(o_pc[x]-o_po[x])*o_scale[x]*o_scale[x];
    dist = sqrt(dist);

    double inveffcenter = fabs(o_pc[1]-o_po[1])/(dist*o_centerpnt[1]);
    for (int x = 2; x <= o_dim; x++) {
        double dum = fabs(o_pc[x]-o_po[x])/(dist*o_centerpnt[x]);
        if (dum > inveffcenter)
             inveffcenter = dum;
    }

    // In 1-D, X=(pc[]-po[])/o_centerpnt[ ]
    // In 2-D, pick effcenter such that above holds for each dimension.
    fprintf(fp, "pc[0]=0\npo[0]=%f\n", dist*inveffcenter);

    // pc on plane, po on the boundary
    // Unscale it.
    for (int x = 1; o_dim >= x; ++x) {
        fprintf(fp, "pc[%i]=%f\n", x, o_pc[x]*o_scale[x]);
        fprintf(fp, "po[%i]=%f\n", x, o_po[x]*o_scale[x]);
    }

    // Print the circuit and circnumber names.
    fprintf(fp, "set circuit = '%s'\n", param.circuit_name());
    fprintf(fp, "set opt_num = '%s'\n", param.opt_num());
    fclose(fp);

    strcpy(system_call, param.spice_name());
    strcat(system_call, BOUNDARY_CALL);
    system(system_call);

    fp = fopen(POINT_FILE,"r");
    if (!fp) {
        strcpy(errmsg, "Can't read the '");
        strcat(errmsg, POINT_FILE);
        strcat(errmsg, "' file.\n");
        nrerror(errmsg);
    }
    int concave;
    fscanf(fp, "%d", &concave);

    // Read in and scale the new point.
    if (!concave) {
        ++o_pntcount;

        // Throw away the zeroeth array element.
        double dum;
        fscanf(fp, "%lf", &dum);
        for (int x = 1; x <= o_dim; x++) {
            fscanf(fp, "%lf", *(o_hullpnts+o_pntcount)+x);
            o_hullpnts[o_pntcount][x] /= o_scale[x];
        }

        for (int x = 0; x <= o_dim; x++) 
            fscanf(fp, "%lf", &o_delta[x]);
    }

    fclose(fp);
    return (!concave);
}


void
OPTIMIZER::add_parameter_set(PARAMETER_SET *newp)
{
    if (o_parameter_list == 0)
        o_end_of_list = o_parameter_list = newp;
    else {
        o_end_of_list->next = newp;
        o_end_of_list = newp;
    }
}

