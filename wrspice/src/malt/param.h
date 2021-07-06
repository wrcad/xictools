#ifndef PARAM_H
#define PARAM_H

#include <math.h>
#include "spnumber/hash.h"
#include "miscutil/lstring.h"

#define NAME_LENGTH 40
#define LINE_LENGTH 100
#define LONG_LINE_LENGTH    512



#define COLLECT_PHASE       0
#define COLLECT_VOLTAGE     1

#define COLLECT_BY_PHASE    0
#define COLLECT_BY_VOLTAGE  1
#define COLLECT_EDGES       2

#define STEP_FORWARD        5

#define RISING_EDGE         0
#define FALLING_EDGE        1

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)<(y))?(y):(x))

// File names.
#define INIT_CONFIG_FILE    "init.config"
#define SPICE_CONFIG_FILE   "config"
#define POINT_FILE          "point"
#define LIMITS_FILE         "limits"

// Parameters.

#define PHASE_THRESHOLD 1.5*M_PI

#define MIN_PHASE_THRESHOLD 1.0*M_PI
#define MAX_PHASE_THRESHOLD 2.0*M_PI
#define INTERPOLATION       1

#define VOLTAGE_THRESHOLD   1.5e-4

#define PHASE_THRESHOLD_MIN 1.5*M_PI

#define PASS_FAIL_THRESHOLD 2.0
#define CHECKING_RATIO      0.9
#define DEFAULT_ACCURACY    0.5
#define DEFAULT_TIME_ACCURACY 0.1
#define MAX_DELAY_VARIATION 20
#define MINIMUM_STEP        0

#define INPUT_DIR           "IN/"
#define TMP_DIR             "TMP/"
#define OUTPUT_DIR          "OUT/"

#define JSPICE_CALL         "spice3"

#define CIRCUIT_EXT         ".cir"
#define PARAM_EXT           ".param"
#define TPARAM_EXT          ".tparam"
#define PRINT_EXT           ".print"
#define INIT_EXT            ".init"
#define PASSFAIL_EXT        ".passfail"

#define STEP_EXT            ".step"
#define CHECK_EXT           ".check"
#define PHASE_EXT           ".phase"
#define AVR_EXT             ".mid"
#define SETUP_EXT           ".setup"
#define HOLD_EXT            ".hold"
#define GOOD_EXT            ".pass"
#define SEPARATION_EXT      ".sep"
#define BAD_EXT             ".fail"
#define NOMINAL_EXT         ".nom"
#define MARGIN_EXT	        ".margins"
#define BOUNDARY_EXT	    ".boundary"
#define OPT_EXT		        ".opt"
#define REGION_EXT          ".region"
#define DELAY_EXT           ".delay"
#define SIGNAL_EXT          ".gen"

#define LOGIC_EXT           ".logic"
#define ASYNC_EXT           ".async"
#define HOLDSETUP_EXT       ".hs"
#define REPORT_EXT          ".report"

#define VALUE_EXT           ".value"
#define VALUE_COMP_EXT      ".value.comp"
#define INTERPOLATE_EXT     ".inter"
#define TIMEINIT_EXT        ".timeinit"

// Latching logic parameters.
#define LATCH_ONE_VOLT      2.0e-3
#define LATCH_THRESHOLD     0.5
#define LATCH_MIN_ONE       0.7
#define LATCH_MAX_ZERO      0.3
#define LATCH_CHECK_INT     100
#define LATCH_EDGE_INT      50
#define LATCH_MIN_EDGE_INT  20
#define LATCH_CHECKRATIO    0.5

typedef double TIME;
typedef double PHASE;
typedef double VOLTAGE;

struct PARAMETERS
{
    bool opt_begin(int, const char*[]);

    int maxplane()              { return (p_maxplane); }
    int maxiter()               { return (p_maxiter); }
    int miniter()               { return (p_miniter); }
    int pulse_extr_method()     { return (p_pulse_extr_method); }
    PHASE nom_phase_thresh()    { return (p_nom_phase_thresh); }
    PHASE min_phase_thresh()    { return (p_min_phase_thresh); }
    PHASE max_phase_thresh()    { return (p_max_phase_thresh); }
    bool interpolate()          { return (p_interpolate); }

    PHASE min_pulse_phase_diff() { return (p_min_pulse_phase_diff); }
    VOLTAGE volt_thresh()       { return (p_volt_thresh); }

    PHASE min_fail_thresh()     { return (p_min_fail_thresh); }
    double checkpoint_ratio()   { return (p_checkpoint_ratio); }
    TIME accuracy()             { return (p_accuracy); }
    TIME time_accuracy()        { return (p_time_accuracy); }
    TIME max_delay_var()        { return (p_max_delay_var); }
    int min_time_step()         { return (p_min_time_step); }
    bool inputs_checked()       { return (p_inputs_checked); }
    int max_index_var()         { return (p_max_index_var); }

    const char *spice_name()    { return (p_spice_name); }
    const char *input_dir()     { return (p_input_dir); }
    const char *tmp_dir()       { return (p_tmp_dir); }
    const char *output_dir()    { return (p_output_dir); }

    const char *circuit_ext()   { return (p_circuit_ext); }
    const char *param_ext()     { return (p_param_ext); }
    const char *print_ext()     { return (p_print_ext); }
    const char *passfail_ext()  { return (p_passfail_ext); }
    const char *phase_ext()     { return (p_phase_ext); }
    const char *step_ext()      { return (p_step_ext); }
    const char *check_ext()     { return (p_check_ext); }
    const char *avr_ext()       { return (p_avr_ext); }
    const char *hold_ext()      { return (p_hold_ext); }
    const char *setup_ext()     { return (p_setup_ext); }
    const char *separation_ext() { return (p_separation_ext); }
    const char *good_ext()      { return (p_good_ext); }
    const char *bad_ext()       { return (p_bad_ext); }
    const char *logic_ext()     { return (p_logic_ext); }
    const char *async_ext()     { return (p_async_ext); }
    const char *hose_ext()      { return (p_hose_ext); }
    const char *report_ext()    { return (p_report_ext); }
    const char *period_ext()    { return (p_period_ext); }
    const char *comp_ext()      { return (p_comp_ext); }
    const char *nom_ext()       { return (p_nom_ext); }
    const char *init_ext()      { return (p_init_ext); }
    const char *marg_ext()      { return (p_marg_ext); }
    const char *opt_ext()       { return (p_opt_ext); }
    const char *bound_ext()     { return (p_bound_ext); }
    const char *region_ext()    { return (p_region_ext); }
    const char *timeinit_ext()  { return (p_timeinit_ext); }
    const char *signal_ext()    { return (p_signal_ext); }

    const char *circuit_name()  { return (p_circuit_name); }
    const char *opt_num()       { return (p_opt_num); }
    int input_options()         { return (p_input_options); }

    double avr_one_volt()       { return (p_avr_one_volt); }
    double nom_edge_thresh()    { return (p_nom_edge_thresh); }
    double max_zero_volt()      { return (p_max_zero_volt); }
    double min_one_volt()       { return (p_min_one_volt); }
    TIME min_edge_sep()         { return (p_min_edge_sep); }
    TIME min_checks_interval()  { return (p_min_checks_interval); }
    TIME min_interval_to_riseedge() { return (p_min_interval_to_riseedge); }
    TIME min_interval_to_falledge() { return (p_min_interval_to_falledge); }
    double clk_chktime_ratio()  { return (p_clk_chktime_ratio); }

    void set_maxplane(int i)            { p_maxplane = i; }
    void set_maxiter(int i)             { p_maxiter = i; }
    void set_miniter(int i)             { p_miniter = i; }
    void set_pulse_extr_method(int i)   { p_pulse_extr_method = i; }
    void set_nom_phase_thresh(PHASE p)  { p_nom_phase_thresh = p; }
    void set_min_phase_thresh(PHASE p)  { p_min_phase_thresh = p; }
    void set_max_phase_thresh(PHASE p)  { p_max_phase_thresh = p; }
    void set_interpolate(bool b)        { p_interpolate = b; }

    void set_min_pulse_phase_diff(PHASE p) { p_min_pulse_phase_diff = p; }
    void set_volt_thresh(VOLTAGE v)     { p_volt_thresh = v; }

    void set_min_fail_thresh(PHASE p)   { p_min_fail_thresh = p; }
    void set_checkpoint_ratio(double d) { p_checkpoint_ratio = d; }
    void set_accuracy(TIME t)           { p_accuracy = t; }
    void set_time_accuracy(TIME t)      { p_time_accuracy = t; }
    void set_max_delay_var(TIME t)      { p_max_delay_var = t; }
    void set_min_time_step(int i)       { p_min_time_step = i; }
    void set_inputs_checked(bool b)     { p_inputs_checked = b; }
    void set_max_index_var(int i)       { p_max_index_var = i; }

    void set_spice_name(const char *s)    { delete [] p_spice_name;
                                            p_spice_name = lstring::copy(s); }
    void set_input_dir(const char *s)     { delete [] p_input_dir;
                                            p_input_dir = lstring::copy(s); }
    void set_tmp_dir(const char *s)       { delete [] p_tmp_dir;
                                            p_tmp_dir = lstring::copy(s); }
    void set_output_dir(const char *s)    { delete [] p_output_dir;
                                            p_output_dir = lstring::copy(s); }

    void set_circuit_ext(const char *s)   { delete [] p_circuit_ext;
                                            p_circuit_ext = lstring::copy(s); }
    void set_param_ext(const char *s)     { delete [] p_param_ext;
                                            p_param_ext = lstring::copy(s); }
    void set_print_ext(const char *s)     { delete [] p_print_ext;
                                            p_print_ext = lstring::copy(s); }
    void set_passfail_ext(const char *s)  { delete [] p_passfail_ext;
                                            p_passfail_ext = lstring::copy(s); }
    void set_phase_ext(const char *s)     { delete [] p_phase_ext;
                                            p_phase_ext = lstring::copy(s); }
    void set_step_ext(const char *s)      { delete [] p_step_ext;
                                            p_step_ext = lstring::copy(s); }
    void set_check_ext(const char *s)     { delete [] p_check_ext;
                                            p_check_ext = lstring::copy(s); }
    void set_avr_ext(const char *s)       { delete [] p_avr_ext;
                                            p_avr_ext = lstring::copy(s); }
    void set_hold_ext(const char *s)      { delete [] p_hold_ext;
                                            p_hold_ext = lstring::copy(s); }
    void set_setup_ext(const char *s)     { delete [] p_setup_ext;
                                            p_setup_ext = lstring::copy(s); }
    void set_separation_ext(const char *s){ delete [] p_separation_ext;
                                            p_separation_ext = lstring::copy(s); }
    void set_good_ext(const char *s)      { delete [] p_good_ext;
                                            p_good_ext = lstring::copy(s); }
    void set_bad_ext(const char *s)       { delete [] p_bad_ext;
                                            p_bad_ext = lstring::copy(s); }
    void set_logic_ext(const char *s)     { delete [] p_logic_ext;
                                            p_logic_ext = lstring::copy(s); }
    void set_async_ext(const char *s)     { delete [] p_async_ext;
                                            p_async_ext = lstring::copy(s); }
    void set_hose_ext(const char *s)      { delete [] p_hose_ext;
                                            p_hose_ext = lstring::copy(s); }
    void set_report_ext(const char *s)    { delete [] p_report_ext;
                                            p_report_ext = lstring::copy(s); }
    void set_period_ext(const char *s)    { delete [] p_period_ext;
                                            p_period_ext = lstring::copy(s); }
    void set_comp_ext(const char *s)      { delete [] p_comp_ext;
                                            p_comp_ext = lstring::copy(s); }
    void set_nom_ext(const char *s)       { delete [] p_nom_ext;
                                            p_nom_ext = lstring::copy(s); }
    void set_init_ext(const char *s)      { delete [] p_init_ext;
                                            p_init_ext = lstring::copy(s); }
    void set_marg_ext(const char *s)      { delete [] p_marg_ext;
                                            p_marg_ext = lstring::copy(s); }
    void set_opt_ext(const char *s)       { delete [] p_opt_ext;
                                            p_opt_ext = lstring::copy(s); }
    void set_bound_ext(const char *s)     { delete [] p_bound_ext;
                                            p_bound_ext = lstring::copy(s); }
    void set_region_ext(const char *s)    { delete [] p_region_ext;
                                            p_region_ext = lstring::copy(s); }
    void set_timeinit_ext(const char *s)  { delete [] p_timeinit_ext;
                                            p_timeinit_ext = lstring::copy(s); }
    void set_signal_ext(const char *s)    { delete [] p_signal_ext;
                                            p_signal_ext = lstring::copy(s); }

    void set_circuit_name(const char *s)  { delete [] p_circuit_name;
                                            p_circuit_name = lstring::copy(s); }
    void set_opt_num(const char *s)       { delete [] p_opt_num;
                                            p_opt_num = lstring::copy(s); }
    void set_input_options(int i)         { p_input_options = i; }

    void set_avr_one_volt(double d)     { p_avr_one_volt = d; }
    void set_nom_edge_thresh(double d)  { p_nom_edge_thresh = d; }
    void set_max_zero_volt(double d)    { p_max_zero_volt = d; }
    void set_min_one_volt(double d)     { p_min_one_volt = d; }
    void set_min_edge_sep(TIME t)       { p_min_edge_sep = t; }
    void set_min_checks_interval(TIME t){ p_min_checks_interval = t; }
    void set_min_interval_to_riseedge(TIME t) { p_min_interval_to_riseedge = t; }
    void set_min_interval_to_falledge(TIME t) { p_min_interval_to_falledge = t; }
    void set_clk_chktime_ratio(double d){ p_clk_chktime_ratio = d; }

private:
    void init_parameters();
    int read_config(const char*);
    int check_option(const char*);
    int read_options(int, const char**);
    int generate_spice_config();

    int p_maxplane;
        // Maximum number of hull planes.

    int p_maxiter;
        // Maximum number of iterations.

    int p_miniter;
        // Maximum number of iterations.

    int p_pulse_extr_method;
        // Pulse extraction method possible values:
        //   COLLECT_BY_PHASE    = phase threshold method
        //   COLLECT_BY_VOLTAGE  = peak voltage method 

    PHASE p_nom_phase_thresh;
        // Phase threshold that determines the exact moment when the
        // pulse apears.

    PHASE p_min_phase_thresh;
        // Minimum expected value of the phase in simulated timepoints
        // closest to the phase threshold.

    PHASE p_max_phase_thresh;
        // Maximum expected value of the phase in simulated timepoints
        // closest to the phase threshold.

    bool p_interpolate;
        // If this variable is set to 1 the time when the value of the
        // signal phase crosses the threshold is determined by the use
        // of interpolation.  If this variable is set to 0 the
        // simulation point with the value of phase closest to the
        // threshold is used.

    VOLTAGE p_volt_thresh;
        // The minimum value of voltage peak that can be treated as
        // the RSFQ pulse.

    PHASE p_min_pulse_phase_diff;
        // The minimum value of difference in phase between two
        // consucutive voltage peaks above which they are treated as
        // two separate pulses.  If the difference in phase is less
        // than minimum the peak with higher voltage level is assumed
        // to be the pulse, the other is ignored.

    double p_checkpoint_ratio;
        // ratio determining the timepoint within the clock cycle for
        // which the passfail test for proper phase is performed.

    PHASE p_min_fail_thresh;
        // Minimum difference between real and expected value of phase
        // threshold considered as test failure.

    TIME p_accuracy;
        // Accuracy in binary search for hold and setup times.

    TIME p_time_accuracy;
        // Timing search accuracy.

    TIME p_max_delay_var;
        // Maximum assumed variation of the output propagation delay
        // resulting from variation of circuit operating parameters.

    int p_max_index_var;
        // Maximum assumed variation of the index corresponding to
        // max_delay_var in delay.

    int p_min_time_step;
        // Min nr of simulation steps between passfail checktimes.

    bool p_inputs_checked;
        // Marker, if inputs are to be checked for extra pulses.

    const char *p_input_dir;
        // Names of subdirectories for input files.

    const char *p_tmp_dir;
        // For temporary files.

    const char *p_output_dir;
        // For output files.

    const char *p_spice_name;
        // Call name for jspice.

    const char *p_circuit_ext;
    const char *p_param_ext;
    const char *p_print_ext;
    const char *p_passfail_ext;
    const char *p_phase_ext;
    const char *p_step_ext;
    const char *p_check_ext;
    const char *p_avr_ext;
    const char *p_hold_ext;
    const char *p_setup_ext;
    const char *p_separation_ext;
    const char *p_good_ext;
    const char *p_bad_ext;
    const char *p_logic_ext;
    const char *p_async_ext;
    const char *p_hose_ext;
    const char *p_report_ext;
    const char *p_period_ext;
    const char *p_comp_ext;
    const char *p_nom_ext;
    const char *p_init_ext;
    const char *p_marg_ext;
    const char *p_opt_ext;
    const char *p_bound_ext;
    const char *p_region_ext;
    const char *p_timeinit_ext;
    const char *p_signal_ext;
        // Extensions.

    const char *p_circuit_name;
        // Circuit name.

    const char *p_opt_num;
        // Parameter set number.

    int p_input_options;
        // Command line options.

  // OPTIONS FOR LATCHING LOGIC

    double p_avr_one_volt;
        // Average voltage in the "1" state.

    double p_nom_edge_thresh;
        // Thresholds used to determine the position of the rising and
        // falling edge of the signal in MVTL logic.

    double p_max_zero_volt;
        // Maximum voltage in the "0" state.

    double p_min_one_volt;
        // Minimum voltage in the "1" state.

    TIME p_min_edge_sep;
        // Minimum separation between rising and falling edge of the
        // signal in the latching logic.

    TIME p_min_checks_interval;
        // Minimum interval between passfail checks.

    TIME p_min_interval_to_riseedge;
        // Minimum interval between checkpoint and the rising edge of
        // the signal for nominal values of parameters.

    TIME p_min_interval_to_falledge;
        // Minimum interval between the falling edge of the signal and
        // the checkpoint for nominal values of parameters.

    double p_clk_chktime_ratio;
        // Ratio determining the timepoint within an interval of high
        // value of the clock signal, when the output signals are
        // checked for the correct value.
};

extern PARAMETERS param;

#endif

