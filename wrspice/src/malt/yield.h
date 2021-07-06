
#ifndef YIELD_H
#define YIELD_H


#define SHUFFLE_LENGTH 98

struct YIELD
{
    YIELD()
    {
        y_s         = 0.0;
        y_it        = 0;
        y_dimens    = 0;
        y_iy        = 0;
        y_iff       = 0;
        y_iset      = 0;
        y_gset      = 0.0;
        for (int i = 0; i < SHUFFLE_LENGTH; i++)
            y_shuff_tab[i] = 0;
    }

    int dimens()    { return (y_dimens); }

    double area(double, double);
    double multiarea(double, int);
    double uniform_deviate(long*);

private:
    double trapzd(double (*)(double), double, double, int);
    double qsimp(double (*)(double), double, double);
    double gauss_deviate(long*);

    double  y_s;
    int     y_it;
    int     y_dimens;
    long    y_iy;
    long    y_shuff_tab[SHUFFLE_LENGTH];
    int     y_iff;
    int     y_iset;
    double  y_gset;
};

extern YIELD yld;

double normal(double);

inline double uniform_deviate(long *idum)
{
    return (yld.uniform_deviate(idum));
}

inline double area(double leftmargin, double rightmargin)
{
    return (yld.area(leftmargin, rightmargin));
}

inline double multiarea(double r, int dim)
{
    return (yld.multiarea(r, dim));
}

#endif

