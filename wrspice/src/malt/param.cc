//XXX
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>

#include "param.h"

// OPTIONS

#define M_SET       0x0001
#define JM_SET      0x0002
#define H_SET       0x0004
#define JH_SET      0x0008
#define S_SET       0x0010
#define JS_SET      0x0020
#define R_SET       0x4000
#define JR_SET      0x8000
#define J_SET       (JM_SET | JH_SET | JS_SET | JR_SET)

#define Q_SET       0x0040
#define V_SET       0x0080

#define P_SET       0x0100

#define C_SET       0x0400
#define B_SET       0x0800
#define TWO_SET     0x1000

#define ID_SET      0x00010000
#define IH_SET      0x00020000
#define IS_SET      0x00040000

#define W_SET       0x00080000

#define BATCH_SET   0x00080000
#define I_SET       (ID_SET | IH_SET | IS_SET)

#define LATCH_SET   0x00100000


PARAMETERS param;


namespace {
    bool read_opt_command_line(int argc, const char *argv[])
    {
        if ((argc < 3) || (argc > 4))
            return (false);

        if (argc == 3) {
            if (argv[1][0]!='-')
                return (true);
            else
                return (false);
        }

        if ((argc == 4) && (argv[2][0] != '-')) {
            if (strcmp(argv[1], "-B") == 0) {
                param.set_input_options(param.input_options() | BATCH_SET);
                return (true);
            }
        }
        return (false);
    }
}


bool
PARAMETERS::opt_begin(int argc, const char *argv[])
{
    // Load filename variables and stuff.
    init_parameters();

    // Check syntax.
    if (!read_opt_command_line(argc, argv)) {
        printf(
            "SYNOPSIS:\n\topt [-B] <circuit_name> <parameter_set_number>\n\n");
        return (false);
    }

    if (!read_config(INIT_CONFIG_FILE)) {
        printf("Can't read the '");
        printf(INIT_CONFIG_FILE);
        printf("' file.\n");
        return (false);
    }

    // Read name of input file.
    set_circuit_name(argv[argc-2]);

    // Read name of input number.
    set_opt_num(argv[argc-1]);

    if (param.input_options() & BATCH_SET) {
        char *t = new char[strlen(param.spice_name()) + 6];
        strcpy(t, param.spice_name());
        strcat(t, " -b ");
        param.set_spice_name(t);
        delete [] t;
    }
    generate_spice_config();
    return (true);
}


void
PARAMETERS::init_parameters()
{
    set_maxplane            ( 50000 );
    set_maxiter             ( 100 );
    set_miniter             ( 10 );
    set_pulse_extr_method   ( COLLECT_BY_PHASE );
    set_nom_phase_thresh    ( PHASE_THRESHOLD );
    set_min_phase_thresh    ( MIN_PHASE_THRESHOLD );
    set_max_phase_thresh    ( MAX_PHASE_THRESHOLD );
    set_interpolate         ( INTERPOLATION );

    set_min_pulse_phase_diff( PHASE_THRESHOLD_MIN );
    set_volt_thresh         ( VOLTAGE_THRESHOLD );

    set_min_fail_thresh     ( PASS_FAIL_THRESHOLD );
    set_checkpoint_ratio    ( CHECKING_RATIO );
    set_accuracy            ( DEFAULT_ACCURACY );
    set_time_accuracy       ( DEFAULT_TIME_ACCURACY );
    set_max_delay_var       ( MAX_DELAY_VARIATION );
    set_min_time_step       ( MINIMUM_STEP );
    set_inputs_checked      ( 1 );
    set_max_index_var       ( 0 );

    set_spice_name          ( lstring::copy(JSPICE_CALL) );
    set_input_dir           ( lstring::copy(INPUT_DIR) );
    set_tmp_dir             ( lstring::copy(TMP_DIR) );
    set_output_dir          ( lstring::copy(OUTPUT_DIR) );

    set_circuit_ext         ( lstring::copy(CIRCUIT_EXT) );
    set_param_ext           ( lstring::copy(PARAM_EXT) );
    set_print_ext           ( lstring::copy(PRINT_EXT) );
    set_step_ext            ( lstring::copy(STEP_EXT) );
    set_check_ext           ( lstring::copy(CHECK_EXT) );
    set_avr_ext             ( lstring::copy(AVR_EXT) );
    set_hold_ext            ( lstring::copy(HOLD_EXT) );
    set_setup_ext           ( lstring::copy(SETUP_EXT) );
    set_separation_ext      ( lstring::copy(SEPARATION_EXT) );
    set_good_ext            ( lstring::copy(GOOD_EXT) );
    set_bad_ext             ( lstring::copy(BAD_EXT) );
    set_logic_ext           ( lstring::copy(LOGIC_EXT) );
    set_hose_ext            ( lstring::copy(HOLDSETUP_EXT) );
    set_report_ext          ( lstring::copy(REPORT_EXT) );
    set_period_ext          ( lstring::copy(INTERPOLATE_EXT) );
    set_comp_ext            ( lstring::copy(VALUE_COMP_EXT) );
    set_phase_ext           ( lstring::copy(PHASE_EXT) );
    set_nom_ext             ( lstring::copy(NOMINAL_EXT) );
    set_init_ext            ( lstring::copy(INIT_EXT) );
    set_passfail_ext        ( lstring::copy(PASSFAIL_EXT) );
    set_marg_ext            ( lstring::copy(MARGIN_EXT) );
    set_bound_ext           ( lstring::copy(BOUNDARY_EXT) );
    set_opt_ext             ( lstring::copy(OPT_EXT) );
    set_region_ext          ( lstring::copy(REGION_EXT) );
    set_timeinit_ext        ( lstring::copy(TIMEINIT_EXT) );
    set_signal_ext          ( lstring::copy(SIGNAL_EXT) );

    set_input_options       ( 0 );

    set_avr_one_volt        ( LATCH_ONE_VOLT );
    set_nom_edge_thresh     ( LATCH_THRESHOLD );
    set_min_edge_sep        ( LATCH_MIN_EDGE_INT );
    set_max_zero_volt       ( LATCH_MAX_ZERO );
    set_min_one_volt        ( LATCH_MIN_ONE );
    set_min_checks_interval ( LATCH_CHECK_INT );
    set_min_interval_to_riseedge ( LATCH_EDGE_INT );
    set_min_interval_to_falledge ( LATCH_EDGE_INT );
    set_clk_chktime_ratio   ( LATCH_CHECKRATIO );
}


namespace {
    // LIST OF VALID CONFIGURATION VARIABLES
    //
    // list of valid configuration variables i.e., such variables which
    // names can appear in the configuration file - hs.config;
    //
    // constants corresponding to each element of the list are given
    // below; any change on this list should be followed by a change of
    // corresponding constants;

    const char *config_item_list[] = {
        "PULSE_EXTRACTION_METHOD",
        "PHASE_THRESHOLD_NOM",
        "PHASE_THRESHOLD_MIN",
        "PHASE_THRESHOLD_MAX",
        "TIME_INTERPOLATION",
        "VOLTAGE_THRESHOLD",

        "MINIMUM_PHASE_DIFFERENCE",
        "CHECKPOINT_RATIO",
        "FAIL_THRESHOLD",
        "INPUT_DIRECTORY",
        "TMP_DIRECTORY",
        "OUTPUT_DIRECTORY",
        "SPICE_CALL_NAME",
        "CIRCUIT_EXTENSION",
        "NOMINAL_PARAMETERS_EXTENSION",
        "PRINT_EXTENSION",
        "STEP_EXTENSION",
        "CHECKPOINTS_EXTENSION",
        "PHASE_EXTENSION",
        "SETUP_EXTENSION",
        "HOLD_EXTENSION",
        "PASS_EXTENSION",
        "FAIL_EXTENSION",
        "LOGIC_EXTENSION",
        "HOLDSETUP_EXTENSION",
        "REPORT_EXTENSION",
        "MID_EXTENSION",
        "BINSEARCH_ACCURACY",
        "INPUTS_CHECKED",
        "MAX_DELAY_VARIATION",
        "MIN_TIME_STEP",
        "NOMINAL_EXTENSION",
        "INIT_EXTENSION",
        "PASSFAIL_EXTENSION",
        "MARGINS_EXTENSION",
        "BOUNDARY_EXTENSION",
        "OPTIMIZATION_EXTENSION",
        "REGION_EXTENSION",
        "SEPARATION_EXTENSION",

        "MAX_PLANE",
        "MAX_ITER",
        "MIN_ITER",

        "TIMING_SEARCH_ACCURACY",
        "SIGNAL_EXTENSION",
        "THRESHOLD_LEVEL",
        "MAX_ZERO_LEVEL",
        "MIN_ONE_LEVEL",
        "INTERVAL_BETWEEN_CHECKPOINTS",
        "RISING_EDGE_CHECKPOINT_INTERVAL",
        "FALLING_EDGE_CHECKPOINT_INTERVAL",
        "MIN_INTEREDGE_INTERVAL",
        "CLOCK_CHECKTIME_RATIO",
        "AVR_ONE_VOLTAGE"
    };

#define CONFIG_ITEM_NUM (sizeof(config_item_list)/sizeof(char*))

    // The following codes describe the position of an appropriate item in
    // config_item_list.

    enum
    {
         PLS_EXTR_MTHD_CONFIG,
         PH_THR_NOM_CONFIG,
         PH_THR_MIN_CONFIG,
         PH_THR_MAX_CONFIG,
         INTERPOL_CONFIG,
         VOLT_THR_CONFIG,

         MIN_PH_DIFF_CONFIG,
         CHECK_RATIO_CONFIG,
         FAIL_THR_CONFIG,
         IN_CONFIG,
         TMP_CONFIG,
         OUT_CONFIG,
         SPICE_CONFIG,
         CIRC_CONFIG,
         PARAM_CONFIG,
         PRINT_CONFIG,
         STEP_CONFIG,
         CHECK_CONFIG,
         PHASE_CONFIG,
         SETUP_CONFIG,
         HOLD_CONFIG,
         GOOD_CONFIG,
         BAD_CONFIG,
         LOGIC_CONFIG,
         HOSE_CONFIG,
         REPORT_CONFIG,
         MID_CONFIG,
         ACCURACY_CONFIG,
         IN_CHECK_CONFIG,
         MAX_DEL_VAR_CONFIG,
         MIN_TIME_STEP_CONFIG,
         NOMINAL_CONFIG,
         INIT_CONFIG,
         PASSFAIL_CONFIG,
         MARGINS_CONFIG,
         BOUNDARY_CONFIG,
         OPTIMIZE_CONFIG,
         REGION_CONFIG,
         SEPARATION_CONFIG,

         MAXPLANE_CONFIG,
         MAXITER_CONFIG,
         MINITER_CONFIG,

         TIME_ACCURACY_CONFIG,
         SIGNAL_CONFIG,
         THRESHOLD_CONFIG,
         MAX_ZERO_CONFIG,
         MIN_ONE_CONFIG,
         CHECK_INT_CONFIG,
         RISEEDGE_CHECK_INT_CONFIG,
         FALLEDGE_CHECK_INT_CONFIG,
         INTEREDGE_INT_CONFIG,
         CLK_CHECKTIME_CONFIG,
         ONE_VOLTAGE_CONFIG,
    };


    // FUNCTION: check_config_item
    // GOAL:
    //  verifying, if an item identifier in the configuration
    //  file is a valid item type;
    //
    // INPUT:
    //  item_type - item identifier being checked;
    //
    // OUTPUT:
    //  item code number (i.e., position on the config_item_list)
    //     - if the item is on the item list, or
    //  CONFIG_ITEM_NUM + 1 (item list size plus one) - if it is invalid;
    //
    // OPERATION:
    //  looking for the config_item_type on the config_item_list;
    //
    // USED IN:
    //  read_config();
    //
    int check_config_item(char *item_type, const char **item_list, int item_num)
    {
        int i = 0;
        int not_found = 1;
        do { 
            not_found = strcmp(item_type, item_list[i++]);
        } while (not_found && (i < item_num));

        if (!not_found)
            return (i-1);
        return(item_num + 1);
    }


    int blank_line(char *line)
    {
        int i = 0, n = strlen(line);
        while (i < n) {
            if (!isspace(line[i++]))
                return (0);
        }
        return (1);
    }


    void unrecognized_value(const char *line, const char *filename)
    {
        printf("Unrecognized value of parameter in %s:\n%s\n\n", filename, line);
    }


    void out_of_range(const char *line, const char *filename)
    {
        printf("Parameter out of range in %s:\n%s\n\n", filename, line);
    }
}


// FUNCTION: read_config
//
// GOAL:
//  reading information about the environment from the
//  file INIT_CONFIG_FILE (="hs.config");
//
// OUTPUT:
//  TRUE - if operation succesful;
//  FALSE - otherwise;
//
int
PARAMETERS::read_config(const char *filename)
{
    char config_item[NAME_LENGTH], item[NAME_LENGTH];
    char in_buffer[LINE_LENGTH];
    char tmpbuf[NAME_LENGTH];

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Warning !!!\nCannot open configuration file %s.\n\n", filename);
        return (0);
    }

    int no_error = 1;
    while (fgets(in_buffer, LINE_LENGTH, fp) != 0) {
        if (in_buffer[0] == '*')
            continue;

        if (blank_line(in_buffer))
            continue;

        {
            // SRW ** Here's an alternate tokenizer that allows
            // interior white space in the right side.  It also makes
            // space arount '=' optional.  This makes it possible to
            // pass arguments to spice_name.

            char *b = in_buffer;
            char *s = config_item;
            while (isspace(*b))
                b++;
            while (*b && !isspace(*b) && *b != '=')
                *s++ = *b++;
            *s = 0;
            while (isspace(*b) || *b == '=')
                b++;
            if (!*b || !*config_item) {
                printf("Unrecognized line in %s:\n%s\n\n", filename, in_buffer);
                no_error = 0;
                continue;
            }
            strncpy(item, b, NAME_LENGTH);
            item[NAME_LENGTH-1] = 0;
            s = item + strlen(item) - 1;
            while (s >= item && isspace(*s))
                *s-- = 0;
        }
        /*********
        if(sscanf(in_buffer, "%s%s%s", config_item, eq, item) != 3) {
            printf("Unrecognized line in %s:\n%s\n\n", filename, in_buffer);
            no_error = 0;
            continue;
        }

        if(strcmp(eq, "=") != 0) {
            printf("Unrecognized line in %s:\n%s\n\n", filename, in_buffer);
            no_error = 0;
            continue;
        }
        *********/

        double fvalue;
        int ivalue;
        switch(
            check_config_item(config_item, config_item_list, CONFIG_ITEM_NUM)) {
        case PLS_EXTR_MTHD_CONFIG:
            if (strcmp(item, "PT") == 0)
                p_pulse_extr_method = COLLECT_BY_PHASE;
            else if (strcmp(item, "VP") == 0)
                p_pulse_extr_method = COLLECT_BY_VOLTAGE;
            else {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            break;

        case PH_THR_NOM_CONFIG:   
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);  
                no_error = 0;
            }   
            else if ((fvalue < 0.0) || (fvalue > 2.0)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_nom_phase_thresh = fvalue*M_PI;
            break;

        case PH_THR_MIN_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);  
                no_error = 0;
            }   
            else if((fvalue < 0.0) || (fvalue > 2.0)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_min_phase_thresh = fvalue*M_PI;
            break;

        case PH_THR_MAX_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);    
                no_error = 0;
            } 
            else if ((fvalue < 0.0) || (fvalue > 2.0)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_max_phase_thresh = fvalue*M_PI;	
            break;

        case INTERPOL_CONFIG:
            if (strcmp(item, "YES") == 0)
                p_interpolate = true;
            else if (strcmp(item, "NO") == 0)
                p_interpolate = false;
            else {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            break;

        case VOLT_THR_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_volt_thresh = fvalue*1e-6;
            break;

        case MAXPLANE_CONFIG:
            if (sscanf(item, "%d", &ivalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_maxplane = ivalue;
            break;

        case MAXITER_CONFIG:
            if (sscanf(item, "%d", &ivalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_maxiter = ivalue;
            break;

        case MINITER_CONFIG:
            if (sscanf(item, "%d", &ivalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_miniter = ivalue;
            break;

        case MIN_PH_DIFF_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);  
                no_error = 0;
            }
            else if ((fvalue < 0.0) || (fvalue > 2.0)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_min_pulse_phase_diff = fvalue*M_PI;	
            break;

        case ACCURACY_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_accuracy = fvalue;
            break;

        case TIME_ACCURACY_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_time_accuracy = fvalue;
            break;

        case MAX_DEL_VAR_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_max_delay_var = fvalue;
            break;

        case MIN_TIME_STEP_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else
                p_min_time_step = fvalue;
            break;

        case CHECK_RATIO_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else if ((fvalue < 0.0) || (fvalue > 1.0)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_checkpoint_ratio = fvalue;
            break;

        case FAIL_THR_CONFIG:
            if (sscanf(item, "%lf", &fvalue) != 1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            else if ((fvalue < 0.0) || (fvalue > 2*M_PI)) {
                out_of_range(in_buffer, filename);
                no_error = 0;
            }
            else
                p_min_fail_thresh = fvalue;
            break;

        case IN_CHECK_CONFIG:
            if (strcmp(item, "YES") == 0)
                p_inputs_checked = true;
            else if (strcmp(item, "NO") == 0)
                p_inputs_checked = false;
            else {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            break;

        case IN_CONFIG:
            strcpy(tmpbuf, item);
            if (item[strlen(item)-1] != '/')
                strcat(tmpbuf, "/");
            delete [] p_input_dir;
            p_input_dir = lstring::copy(tmpbuf);
            break;

        case TMP_CONFIG:
            strcpy(tmpbuf, item);
            if (item[strlen(item)-1] != '/')
                strcat(tmpbuf, "/");
            delete [] p_tmp_dir;
            p_tmp_dir = lstring::copy(tmpbuf);
            break;

        case OUT_CONFIG:
            strcpy(tmpbuf, item);
            if (item[strlen(item)-1] != '/')
                strcat(tmpbuf, "/");
            delete [] p_output_dir;
            p_output_dir = lstring::copy(tmpbuf);
            break;

        case SPICE_CONFIG:
            delete [] p_spice_name;
            p_spice_name = lstring::copy(item);
            break;

        case CIRC_CONFIG:
            delete [] p_circuit_ext;
            p_circuit_ext = lstring::copy(item);
            break;

        case PARAM_CONFIG:
            delete [] p_param_ext;
            p_param_ext = lstring::copy(item);
            break;

        case CHECK_CONFIG:
            delete [] p_check_ext;
            p_check_ext = lstring::copy(item);
            break;

        case STEP_CONFIG:
            delete [] p_step_ext;
            p_step_ext = lstring::copy(item);
            break;

        case PRINT_CONFIG:
            delete [] p_print_ext;
            p_print_ext = lstring::copy(item);
            break;

        case MID_CONFIG:
            delete [] p_avr_ext;
            p_avr_ext = lstring::copy(item);
            break;

        case PHASE_CONFIG:
            delete [] p_phase_ext;
            p_phase_ext = lstring::copy(item);
            break;

        case SETUP_CONFIG:
            delete [] p_setup_ext;
            p_setup_ext = lstring::copy(item);
            break;

        case HOLD_CONFIG:
            delete [] p_hold_ext;
            p_hold_ext = lstring::copy(item);
            break;

        case SEPARATION_CONFIG:
            delete [] p_separation_ext;
            p_separation_ext = lstring::copy(item);
            break;

        case GOOD_CONFIG:
            delete [] p_good_ext;
            p_good_ext = lstring::copy(item);
            break;

        case BAD_CONFIG:
            delete [] p_bad_ext;
            p_bad_ext = lstring::copy(item);
            break;

        case LOGIC_CONFIG:
            delete [] p_logic_ext;
            p_logic_ext = lstring::copy(item);
            break;

        case HOSE_CONFIG:
            delete [] p_hose_ext;
            p_hose_ext = lstring::copy(item);
            break;

        case REPORT_CONFIG:
            delete [] p_report_ext;
            p_report_ext = lstring::copy(item);
            break;

        case NOMINAL_CONFIG:
            delete [] p_nom_ext;
            p_nom_ext = lstring::copy(item);
            break;

        case INIT_CONFIG:
            delete [] p_init_ext;
            p_init_ext = lstring::copy(item);
            break;

        case PASSFAIL_CONFIG:
            delete [] p_passfail_ext;
            p_passfail_ext = lstring::copy(item);
            break;

        case MARGINS_CONFIG:
            delete [] p_marg_ext;
            p_marg_ext = lstring::copy(item);
            break;

        case BOUNDARY_CONFIG:
            delete [] p_bound_ext;
            p_bound_ext = lstring::copy(item);
            break;

        case OPTIMIZE_CONFIG:
            delete [] p_opt_ext;
            p_opt_ext = lstring::copy(item);
            break;

        case REGION_CONFIG:
            delete [] p_region_ext;
            p_region_ext = lstring::copy(item);
            break;

        case SIGNAL_CONFIG:
            delete [] p_signal_ext;
            p_signal_ext = lstring::copy(item);
            break;

        case ONE_VOLTAGE_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_avr_one_volt = fvalue*1.0e-3;
            break;

        case THRESHOLD_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_nom_edge_thresh = fvalue;
            break;

        case MAX_ZERO_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_max_zero_volt = fvalue;
            break;

        case MIN_ONE_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_min_one_volt = fvalue;
            break;

        case CHECK_INT_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_min_checks_interval = fvalue*1.0e-12;
            break;

        case RISEEDGE_CHECK_INT_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_min_interval_to_riseedge = fvalue*1.0e-12;
            break;

        case FALLEDGE_CHECK_INT_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_min_interval_to_falledge = fvalue*1.0e-12;
            break;

        case INTEREDGE_INT_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_min_edge_sep = fvalue*1.0e-12;
            break;

        case CLK_CHECKTIME_CONFIG:
            if (sscanf(item, "%lf", &fvalue)!=1) {
                unrecognized_value(in_buffer, filename);
                no_error = 0;
            }
            p_clk_chktime_ratio = fvalue;
            break;

        default:
            printf("Unknown option in %s:\n%s\n\n", filename, in_buffer);
            no_error = 0;
            break;
        }
    }

    fclose(fp);

    p_nom_edge_thresh *= p_avr_one_volt;
    p_min_one_volt *= p_avr_one_volt;
    p_max_zero_volt *= p_avr_one_volt;

    return (no_error);
}



int
PARAMETERS::check_option(const char *option)
{
    if (option[0] != '-') {
        printf("Unrecognized option: %s\n", option);
        return 0;
    }

    int jset = 0;
    int iset = 0;
    int n = strlen(option);
    for (int k = 1; k < n; k++) {
        switch(option[k]) {
        case 'm': 
            if (jset)
                p_input_options |= JM_SET;
            else
                p_input_options |= M_SET;
            break;

        case 'h':
            if (iset)
                p_input_options |= IH_SET;
            else {
                p_input_options |= H_SET;
                if (jset)
                    p_input_options |= JH_SET;
            }
            break;

        case 'r':
            p_input_options |= R_SET;
            if (jset)
                p_input_options |= JR_SET;
            break;	 

        case 's':
            if (iset)
                p_input_options |= IS_SET;
            else {
                p_input_options |= S_SET;
                if (jset)
                    p_input_options |= JS_SET;
            }
            break;

        case 'j':
            jset = 1;
            break;

        case 'i':
            iset = 1;
            break;

        case 'q':
            p_input_options |= Q_SET;
            break;

        case 'p':
            p_input_options |= P_SET;
            break;

        case 'v':
            p_input_options |= V_SET;
            break;

        case 'w':
            p_input_options |= W_SET;
            break;

        case 'd':
            if (iset)
                p_input_options |= ID_SET;
            else {
                  printf("Unrecognized option: %s\n", option);
                  return 0;
            }
            break;

        case 'B':
            p_input_options |= BATCH_SET;
            break;

        default:
            printf("Unrecognized option: %s\n", option);
            return 0;
        }
    }
    return (1);
}


int
PARAMETERS::read_options(int argc, const char *argv[])
{ 
    if (argc < 2)
        return (0);

    int ret_val = 1;
    int iset = 0;
    for (int i = 1; i < argc-1; i++) {
        if (!check_option(argv[i]))
            ret_val = 0;
        if ((p_input_options & I_SET) && (!iset)) {
            delete [] p_circuit_name;
            p_circuit_name = lstring::copy(argv[argc-2]);
            delete [] p_opt_num;
            p_opt_num = lstring::copy(argv[argc-1]);
            iset = 1;
            argc--;
        }
    }

    if (!iset) {
        delete [] p_circuit_name;
        p_circuit_name = lstring::copy(argv[argc-1]);
    }

    if (!((p_input_options & V_SET) && (p_input_options & P_SET))) {
        if (p_input_options & V_SET)
            p_pulse_extr_method = COLLECT_BY_VOLTAGE;

         if (p_input_options & P_SET)
             p_pulse_extr_method = COLLECT_BY_PHASE;
    }
    return (ret_val);
}


int
PARAMETERS::generate_spice_config()
{
    int is_ws = (strstr(p_spice_name, "wrspice") != 0);

    FILE *fp = fopen(SPICE_CONFIG_FILE, "w");
    if (fp == 0) {
        printf("Cannot create %s configuration file %s.\n\n",
            is_ws ? "wrspice" : "jspice", SPICE_CONFIG_FILE);
        return 0;
    }

    if (is_ws) {
        fprintf(fp, "set subc_catchar=\":\"\n");
        fprintf(fp, "set subc_catmode=\"spice3\"\n\n");
    }

    fprintf(fp, "set circuit = '%s'\n\n", p_circuit_name);

    fprintf(fp, "quiet = %d\n\n", ((p_input_options & Q_SET)?1:0));

    fprintf(fp, "set circext  = '%s'\n", p_circuit_ext);
    fprintf(fp, "set paramext = '%s'\n", p_param_ext);
    fprintf(fp, "set printext = '%s'\n", p_print_ext);
    fprintf(fp, "set passfailext = '%s'\n\n", p_passfail_ext);
    fprintf(fp, "set checkext = '%s'\n", p_check_ext);

    fprintf(fp, "set phasext  = '%s'\n", p_phase_ext);
    fprintf(fp, "set avrext   = '%s'\n", p_avr_ext);
    fprintf(fp, "set holdext  = '%s'\n", p_hold_ext);
    fprintf(fp, "set setupext = '%s'\n", p_setup_ext);
    fprintf(fp, "set sepext = '%s'\n",   p_separation_ext);
    fprintf(fp, "set goodext  = '%s'\n", p_good_ext);
    fprintf(fp, "set badext   = '%s'\n", p_bad_ext);
    fprintf(fp, "set tparext   = '%s'\n\n", TPARAM_EXT);
    fprintf(fp, "set nomext   = '%s'\n\n", p_nom_ext);

    fprintf(fp, "set reportext   = '%s'\n\n", p_report_ext);

    fprintf(fp, "set input_dir   = '%s'\n", p_input_dir);
    fprintf(fp, "set tmp_dir     = '%s'\n", p_tmp_dir);
    fprintf(fp, "set output_dir  = '%s'\n\n", p_output_dir);

    fprintf(fp, "failthres    = %.2f\n", p_min_fail_thresh);
    fprintf(fp, "accuracy     = %.5f\n", p_accuracy/100.0);
    fprintf(fp, "time_accuracy  = %.5f\n", p_time_accuracy);
    fprintf(fp, "delay_variation = %.2f\n", p_max_delay_var);
    fprintf(fp, "check_inputs = %d\n", p_inputs_checked);

    fclose(fp);

    return (1);
}


