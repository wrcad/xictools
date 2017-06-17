
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
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: trnames.h,v 2.2 2015/11/19 04:49:08 stevew Exp $
 *========================================================================*/

#ifndef TRNAMES_H
#define TRNAMES_H

// Soft-wired names for trial specification.  These can be overridden
// by a variable of the same name.
//
extern const char *check_value1;
extern const char *check_value2;
extern const char *check_value;
extern const char *checkN1;
extern const char *checkN2;

// Struct to hold special input vector names for trial iterations.
//
struct sNames
{
    sNames()
        {
            n_value1 = 0;
            n_value2 = 0;
            n_value = 0;
            n_n1 = 0;
            n_n2 = 0;
        }   

    ~sNames()  
        {
            delete [] n_value1;
            delete [] n_value2;
            delete [] n_value;
            delete [] n_n1;
            delete [] n_n2;
        }

    const char *value1()        { return (n_value1); }
    const char *value2()        { return (n_value2); }
    const char *value()         { return (n_value); }
    const char *n1()            { return (n_n1); }
    const char *n2()            { return (n_n2); }

    void set_value1(const char *v)
        {
            char *s = lstring::copy(v);
            delete [] n_value1;
            n_value1 = s;
        }

    void set_value2(const char *v)
        {
            char *s = lstring::copy(v);
            delete [] n_value2;
            n_value2 = s;
        }

    void set_value(const char *v)
        {
            char *s = lstring::copy(v);
            delete [] n_value;
            n_value = s;
        }

    void set_n1(const char *v)
        {
            char *s = lstring::copy(v);
            delete [] n_n1;
            n_n1 = s;
        }

    void set_n2(const char *v)
        {
            char *s = lstring::copy(v);
            delete [] n_n2;
            n_n2 = s;
        }

    void   set_input(sFtCirc*, sPlot*, double, double);
    void   setup_newplot(sFtCirc*, sPlot*);
    static sNames *set_names();

private:
    char *n_value1;
    char *n_value2;
    char *n_value;
    char *n_n1;
    char *n_n2;
};

#endif

