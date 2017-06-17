
#ifndef SCRKIT_H
#define SCRKIT_H

#include "miscmath.h"

//
// Some misc. definitions from Xic.
//

// The physical and electrical resolutions.  Electrical is always 1000.
// Physical defaults to 1000, but can be changed.  Best not to change
// this in user code.
//
extern int CDphysResolution;
extern const int CDelecResolution;

// Internal dimension to microns.
inline double MICRONS(int x) { return (((double)x)/CDphysResolution); }
inline double ELEC_MICRONS(int x) { return (((double)x)/CDelecResolution); }

// Microns to internal dimension.
inline int INTERNAL_UNITS(double d) { return (mmRnd(d*CDphysResolution)); }
inline int ELEC_INTERNAL_UNITS(double d) { return (mmRnd(d*CDelecResolution)); }

// Maximum allowed call hierarchy depth.
#define CDMAXCALLDEPTH      40

// Physical or electrical mode.
enum DisplayMode { Physical=0, Electrical };

#define NO_EXTERNAL

#include "si_if_variable.h"
#include "si_args.h"
#include "si_scrfunc.h"

#endif

