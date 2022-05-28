#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "qnrutil.h"
#include "yield.h"

#define DEBUG  0

YIELD yld;

#define FUNC(x) ((*func)(x))

double
YIELD::trapzd(double (*func)(double), double a, double b, int n)
{
    if (n == 1) {
        y_it = 1;
        return (y_s = 0.5*(b-a)*(FUNC(a)+FUNC(b)));
    }
    else {
        double tnm = y_it;
        double del = (b-a)/tnm;
        double x = a + 0.5*del;
        int j;
        double sum;
        for (sum = 0.0, j = 1; j <= y_it; j++, x += del)
            sum += FUNC(x);
        y_it *= 2;
        y_s = 0.5*(y_s+(b-a)*sum/tnm);
        return (y_s);
    }
}


#define EPS 1.0e-6
#define JMAX 20

double
YIELD::qsimp(double (*func)(double), double a, double b)
{
    double os = -1.0e30;
    double ost = os;
    for (int j = 1; j <= JMAX; j++) {
        double st = trapzd(func, a, b, j);
        double s = (4.0*st - ost)/3.0;
        if (fabs(s-os) < EPS*fabs(os))
            return (s);
        os = s;
        ost = st;
    }
    nrerror("Too many steps in routine QSIMP");
    // not reached
    return (0.0);
}

#undef EPS
#undef JMAX


double normal(double x)
{
    double y = -x*x/2;
    y = exp(y)/sqrt(2*M_PI);
    return (y);
}


double
YIELD::area(double leftmargin, double rightmargin)
{
    return qsimp(normal, 0, leftmargin) + qsimp(normal, 0, rightmargin);
}


double multinormal(double x)
{
    double y = -x*x/2;
    double mx = 1.0;
    for (int i = 0; i < yld.dimens()-1; i++)
        mx *= x;
    y = mx*exp(y);
    return (y);
}


namespace {
    int factorial(int n)
    {
        if (n <= 1)
            return (1);
        return (n*factorial(n-1));
    }

    int double_factorial(int n)
    {
        if (n <= 1)
            return (1);
        return (n*factorial(n-2));
    }

    long int power2(int n)
    {
        long int ny = 1;
        while (n--)
            ny *= 2;
        return (ny);
    }

    double multicoeff(int n)
    {
        int s = (n-1)/2;
        int odd = (n-1)%2;
        if (odd)
            return (1.0/(power2(s)*factorial(s)));
        return (sqrt(2/M_PI)/double_factorial(2*s-1));
    }
}


double
YIELD::multiarea(double r, int dim)
{
    y_dimens = dim;
    double integral = qsimp(multinormal, 0, r);
    double coeff = multicoeff(dim);
    return (integral*coeff);
}


#define IM  714025
#define IA    4096
#define IC   54773

// Return a uniform random deviate between 0.0 and 1.0.  Set idum to
// any negative value to initialize or reinitialize the sequance.
//
double
YIELD::uniform_deviate(long *idum)
{
    if ((*idum < 0) || (y_iff == 0)) {
        y_iff = 1;
        if ((*idum = (IC-(*idum)) % IM) < 0)
            *idum = -(*idum);

        // initialize the shuffle table
        for (int j = 1; j <= SHUFFLE_LENGTH-1; j++) {
            *idum = (IA*(unsigned long)(*idum)+IC) % IM;
            y_shuff_tab[j] = (*idum);
        }
        *idum = (IA*(unsigned long)(*idum)+IC) % IM;
        y_iy = (*idum);
    }

    int j = 1 + (SHUFFLE_LENGTH-1)*y_iy/IM;
    if ((j > SHUFFLE_LENGTH-1) || (j < 0))
        nrerror("Error in random number generator uniform_deviate.");

    y_iy = y_shuff_tab[j];

    *idum = (IA*(unsigned long)(*idum)+IC) % IM;
    y_shuff_tab[j] = *idum;

    return ((double)y_iy/IM);
}


// rightReturns a normally distributed deviate with zero mean and unit
// variance, using uniform_deviate() as the source of uniform
// deviates.
//
double
YIELD::gauss_deviate(long *idum)
{
    if (y_iset == 0) {
        double r, v1, v2;
        do {
            // pick two uniform numbers in the square extending from
            // -1.0 to +1.0 in each direction

            v1 = 2.0*uniform_deviate(idum) - 1.0;
            v2 = 2.0*uniform_deviate(idum) - 1.0;
            r = v1*v1 + v2*v2;
        } while(r >= 1.0);

        // Now make the Box-Muller transformation to get two normal
        // deviates.  Return one and save the other for next time.

        double fac = sqrt(-2.0*log(r)/r);

        y_gset = v1*fac;
        y_iset = 1;
        return (v2*fac);
    }
    else {
        y_iset = 0;
        return (y_gset);
    }
}


#if DEBUG
main()
{
    double lm, rm;

    printf("left margin=");
    scanf("%lf", &lm);
    printf("right margin=");
    scanf("%lf", &rm);
    printf("... %6.4f %6.4f\n",lm,rm);
    printf("the answer is ... %6.4f\n",area(lm,rm));
}
#endif

