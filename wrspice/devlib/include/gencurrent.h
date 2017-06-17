
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: gencurrent.h,v 2.9 2014/04/08 05:47:23 stevew Exp $
 *========================================================================*/

// This is a compact and specialized sparse matrix class for use in
// computing current flow into devices.  For dc/transient, the current
// flow for node i is
// 
//   Ii = AijVj - Ri
// 
// where Aij is the MNA matrix for the device, Vj is the reault vector
// of node voltages, and Ri is the RHS vector containing the terms
// added when the device is loaded.  For ac analysis, Ri is not
// present, and the values are complex.

// Structure containing the MNA matrix and the RHS terms
//
struct dvaMatrix
{
    // List element for a vector
    //
    struct ElemList
    {
        ElemList(int c, ElemList *n)
            {
                real = imag = 0.0;
                indx = c;
                next = n;
            }

        void free();

        double real, imag;  // the data values
        int indx;           // node number
        ElemList *next;
    };

    // List element for heads of vectors
    //
    struct HeadList
    {
        HeadList(int r, HeadList *n)
            {
                row = r;
                head = 0;
                next = n;
            }

        ~HeadList()
            {
                head->free();
            }

        void free();

        int row;            // node number
        ElemList *head;     // row vector head
        HeadList *next;
    };

    dvaMatrix()
        {
            dva_rows = 0;
            dva_rvec = 0;
        }

    ~dvaMatrix()
        {
            dva_rows->free();
            dva_rvec->free();
        }

    void clear();
    double *get_elem(int, int);
    double *get_elem(int);
    double compute_real(int, double*);
    void compute_cplx(int, double*, double*, double*, double*);

private:
    HeadList *dva_rows;     // MNA matrix
    ElemList *dva_rvec;     // RHS terms
};

