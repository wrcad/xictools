// Linear programing for special case of equality constriants m = m3.
// From numerical recipes.

#include <math.h> 
#include "qnrutil.h"
#include "optimizer.h"

#define EPS1 1.0e-9
#define EPS2 1.0e-9


namespace {
    void simp1(double **a, int mm, int *ll, int nll, int iabf, int *kp,
        double *bmax)
    {
        *kp = ll[1];
        *bmax = a[mm+1][*kp+1];
        for (int k = 2; k <= nll; k++) {
            double test;
            if (iabf == 0)
                test = a[mm+1][ll[k]+1] - (*bmax);
            else
                test = fabs(a[mm+1][ll[k]+1]) - fabs(*bmax);
            if (test > 0.0) {
                *bmax = a[mm+1][ll[k]+1];
                *kp = ll[k];
            }
        }
    }

    void simp2(double **a, int n, int *l2, int nl2, int *ip, int kp, double *q1)
    {
        *ip = 0;
        for (int i = 1; i <= nl2; i++) {
            if (a[l2[i]+1][kp+1] < -EPS2) {
                *q1 = -a[l2[i]+1][1]/a[l2[i]+1][kp+1];
                *ip = l2[i];
                for (i = i+1; i <= nl2; i++) {
                    int ii = l2[i];
                    if (a[ii+1][kp+1] < -EPS2) {
                        double q = -a[ii+1][1]/a[ii+1][kp+1];
                        if (q < *q1) {
                            *ip=ii;
                            *q1=q;
                        }
                        else if (q == *q1) {
                            double q0 = 0.0, qp = 0.0;
                            for (int k = 1; k <= n; k++) {
                                qp = -a[*ip+1][k+1]/a[*ip+1][kp+1];
                                q0 = -a[ii+1][k+1]/a[ii+1][kp+1];
                                if (q0 != qp)
                                    break;
                            }
                            if (q0 < qp)
                                *ip = ii;
                        }
                    }
                }
            }
        }
    }

    void simp3(double **a, int i1, int k1, int ip, int kp)
    {
        double piv = 1.0/a[ip+1][kp+1];
        for (int ii = 1; ii <= i1+1; ii++) {
            if (ii-1 != ip) {
                a[ii][kp+1] *= piv;
                for (int kk = 1; kk <= k1+1; kk++) {
                    if (kk-1 != kp)
                        a[ii][kk] -= a[ip+1][kk]*a[ii][kp+1];
                }
            }
        }
        for (int kk = 1; kk <= k1+1; kk++) {
            if (kk-1 != kp)
                a[ip+1][kk] *= -piv;
        }
        a[ip+1][kp+1] = piv;
    }
}


// Static function
int
OPTIMIZER::simplx(double **a, int m, int n, int *izrov, int *iposv)
{
    int *l1 = ivector(1, n+1);
    int *l2 = ivector(1, m);
    int nl1 = n;
    for (int k = 1; k <= n; k++)
        l1[k] = izrov[k] = k;
    int nl2 = m;
    for (int i = 1; i <= m; i++) {
        if (a[i+1][1] < 0.0)
            nrerror("Bad input tableau in SIMPLX");
        l2[i] = i;
        iposv[i] = n+i;
    }

    // stage one
    int ir = 1;
    for (int k = 1; k <= (n+1); k++) {
        double q1 = 0.0;
        for (int i = 1; i <= m; i++)
            q1 += a[i+1][k];
        a[m+2][k] = -q1;
    }
    do {
        int kp;
        double bmax;
        simp1(a, m+1, l1, nl1, 0, &kp, &bmax);
        if (bmax <= EPS1 && a[m+2][1] < -EPS1) {
            free_ivector(l2, 1, m);
            free_ivector(l1, 1, n+1);
            return (-1);
        }
        else if (bmax <= EPS1 && a[m+2][1] <= EPS1) {
            for (int ip = 1; ip <= m; ip++) {
                if (iposv[ip] == (ip+n)) {
                    simp1(a, ip, l1, nl1, 1, &kp, &bmax);
                    if (bmax > 0.0)
                        goto one;
                }
            }
            ir = 0;
            break;
        }
        int ip;
        double q1;
        simp2(a, n, l2, nl2, &ip, kp, &q1);
        if (ip == 0) {
            free_ivector(l2, 1, m);
            free_ivector(l1, 1, n+1);
            return -1;
        }
one:
        simp3(a, m+1, n, ip, kp);
        if (iposv[ip] >= (n+1)) {
            int k;
            for (k = 1; k <= nl1; k++)
                if (l1[k] == kp)
                    break;
            --nl1;
            for (int is = k; is <= nl1; is++)
                l1[is] = l1[is+1];
            a[m+2][kp+1] += 1.0;
            for (int i = 1; i <= m+2; i++)
                a[i][kp+1] = -a[i][kp+1];
        }
        int is = izrov[kp];
        izrov[kp] = iposv[ip];
        iposv[ip] = is;
    } while (ir);

    // phase two
    for (;;) {
        int kp;
        double bmax;
        simp1(a, 0, l1, nl1, 0, &kp, &bmax);
        if (bmax <= 0.0) {
            free_ivector(l2, 1, m);
            free_ivector(l1, 1, n+1);
            return 0;
        }
        int ip;
        double q1;
        simp2(a, n, l2, nl2, &ip, kp, &q1);
        if (ip == 0) {
            free_ivector(l2, 1, m);
            free_ivector(l1, 1, n+1);
            return 1;
        }
        simp3(a, m, n, ip, kp);
        int is = izrov[kp];
        izrov[kp] = iposv[ip];
        iposv[ip] = is;
    }
}

