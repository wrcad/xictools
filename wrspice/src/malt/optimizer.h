
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <stdio.h>

#define MINIMUM_ALPHA_DIVIDER 720

#define NAME_LENGTH 40
#define LINE_LENGTH 100
#define LONG_LINE_LENGTH    512


struct PARAMETER_SET
{
    int x;
    int y;
    int points_nr;
    PARAMETER_SET *next;
};


struct OPTIMIZER
{
    OPTIMIZER()
        {
            o_facecenter    = 0;
            o_radius        = 0;
            o_direction     = 0;
            o_pntstack      = 0;
            o_pc            = 0;
            o_po            = 0;

            o_iterate       = 0;
            o_direcount     = 0;

            o_dim           = 0;
            o_var           = 0;
            o_pin           = 0;
            o_pntcount      = 0;
            o_xyz           = 0;
            o_statics       = 0;
            o_yield         = 0;
            o_startpnt      = 0;
            o_centerpnt     = 0;
            o_lower         = 0;
            o_upper         = 0;
            o_scale         = 0;
            o_delta         = 0;
            o_hullpnts      = 0;
            o_names         = 0;

            o_parameter_list = 0;
            o_end_of_list   = 0;
            o_abpnts        = 0;
            o_ab            = 0;
            o_plncount      = 0;
            o_maxplane      = 10;
            o_stop          = 0;
            o_fpout         = 0;
            o_aNmatrix      = 0;
            o_temp          = 0;
        }

    // optimize.cc
    int opt_begin();
    int opt_loop();
    int opt_result();

    // points.cc
    void readinit();
    bool set_dim(int);
    bool set_param_name(int, const char*);
    bool set_nominal(int, double);
    bool set_lower(int, double);
    bool set_upper(int, double);
    bool set_scale(int, double);
    bool set_yield(int, int);
    bool set_statics(int, int);
    void finalize_setup();
    int addpoint();

private:

    // optimize.cc
    void postcenter(int*, double*);
    int findface(double*, double*);
    void intpickpnts(int, int*);
    void makeaplane(int*);
    void make_more_mem();
    static double det(double**, int);
    static int simplx(double**, int, int, int*, int*);
    void out_screen_file(const char*, ...);

    // points.cc
    void add_parameter_set(PARAMETER_SET*);

    double  *o_facecenter;
    double  *o_radius;
    double  *o_direction;
    int     *o_pntstack;
    double  *o_pc;
    double  *o_po;

    int     o_iterate;
    int     o_direcount;

    int     o_dim;
    int     o_var;
    int     o_pin;
    int     o_pntcount;
    int     *o_xyz;
    int     *o_statics;
    int     *o_yield;
    double  *o_startpnt;
    double  *o_centerpnt;
    double  *o_lower;
    double  *o_upper;
    double  *o_scale;
    double  *o_delta;
    double  **o_hullpnts;
    const char **o_names;

    PARAMETER_SET *o_parameter_list;
    PARAMETER_SET *o_end_of_list;
    int     **o_abpnts;
    double  **o_ab;
    int     o_plncount;
    int     o_maxplane;
    int     o_stop;
    FILE    *o_fpout;
    double  **o_aNmatrix;
    double  *o_temp;
};

#endif

