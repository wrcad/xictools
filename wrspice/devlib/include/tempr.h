//=================================================================
// Class to compute superconductor properties as a function of
// temperature.  Presently, this will compute the order parameter.
//
// Stephen R. Whiteley, wrcad.com,  Synopsys, Inc.  7/7/2021
//=================================================================
// Released under Apache License, Version 2.0.
//   http://www.apache.org/licenses/LICENSE-2.0
//
// References:
// Tinkham, "Introduction to Superconductiovity".
// https://physics.stackexchange.com/questions/54200/superconducting-gap-
//  temperature-dependence-how-to-calculate-this-integral
// https://en.wikipedia.org/wiki/Romberg%27s_method
// http://www.knowledgedoor.com/2/elements_handbook/debye_temperature.html

#ifndef TEMPR_H
#define TEMPR_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef USE_GSL
#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#endif

#define BOLTZ 1.3806226e-23
#define ECHG 1.602176634e-19

// Defaults for niobium.
#define DEBYE_TEMP_NB 276
#define TC_NB 9.2


struct tempr
{
    struct tprms
    {
        double T;
        double del;
    };

    tempr(double Tc = 0.0, double Tdb = 0.0)
        {
            if (Tdb > 0.0)
                t_dbe = Tdb * BOLTZ;
            else
                t_dbe = DEBYE_TEMP_NB * BOLTZ;
            if (Tc > 0.0)
                t_tc = Tc;
            else
                t_tc = TC_NB;
            t_eta = -log((t_tc * BOLTZ)/(1.134*t_dbe));
#ifdef USE_GSL
            t_ws = gsl_integration_workspace_alloc(200);
#endif
        }

#ifdef USE_GSL
    ~tempr()
        {
            gsl_integration_workspace_free(t_ws);
        }
#endif

    double order_parameter(double);


private:
    double t_tc;    // Transition temperature
    double t_dbe;   // Debye energy
    double t_eta;   // 1/N(0)V
#ifdef USE_GSL
    gsl_integration_workspace *t_ws;
#endif
};

#endif

