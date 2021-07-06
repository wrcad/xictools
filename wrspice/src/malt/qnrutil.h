
#ifndef QNRUTIL_H
#define QNRUTIL_H

void nrerror(const char*);

namespace qnrutil {

    inline int *ivector(int nl, int nh)
    {
        int *v = new int[nh-nl+1];
        return (v - nl);
    }

    inline double *vector(int nl, int nh)
    {
        double *v = new double[nh-nl+1];
        return (v-nl);
    }

    inline int **imatrix(int nrl, int nrh, int ncl, int nch)
    {
        int **m = new int*[nrh-nrl+1];
        m -= nrl;
        for (int i = nrl; i <= nrh; i++) {
            m[i] = new int[nch-ncl+1];
            m[i] -= ncl;
        }
        return (m);
    }
     
    inline double **matrix(int nrl, int nrh, int ncl, int nch)
    {
        double **m = new double*[nrh-nrl+1];
        m -= nrl;
        for (int i = nrl; i <= nrh; i++) {
            m[i] = new double[nch-ncl+1];
            m[i] -= ncl;
        }
        return (m);
    }

    inline int **ireallocmatrix(int **oldm, int nrl, int nrh, int newnrh,
        int ncl, int nch)
    {
        int **m = new int*[newnrh-nrl+1];
        m -= nrl;
        for (int i = nrl; i <= nrh; i++)
            m[i] = oldm[i];

        for (int i = nrh+1; i <= newnrh; i++) {
            m[i] = new int[nch-ncl+1];
            m[i] -= ncl;
        }
        delete [] &oldm[nrl];
        return (m);
    }
     
    inline double **reallocmatrix(double **oldm, int nrl, int nrh, int newnrh,
        int ncl, int nch)
    {
        double **m = new double*[newnrh-nrl+1];
        m -= nrl;
        for (int i = nrl; i <= nrh; i++)
            m[i] = oldm[i];

        for (int i = nrh+1; i <= newnrh; i++) {
            m[i] = new double[nch-ncl+1];
            m[i] -= ncl;
        }
        delete [] &oldm[nrl];
        return (m);
    }

    inline void free_ivector(int *v, int nl, int)
    {
        delete &v[nl];
    }

    inline void free_vector(double *v, int nl, int)
    {
        delete &v[nl];
    }

    inline void free_imatrix(int **m, int nrl, int nrh, int ncl, int)
    {
        for (int i = nrh; i >= nrl; i--) {
            delete &m[i][ncl];
        }
        delete &m[nrl];
    }
     
    inline void free_matrix(double **m, int nrl, int nrh, int ncl, int)
    {
        for (int i = nrh; i >= nrl; i--) {
            delete &m[i][ncl];
        }
        delete &m[nrl];
    }

}

using namespace qnrutil;

#endif

