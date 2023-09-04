//////////////
// Differernt modes for the entry
//
enum EntryMode { KW_NORMAL, KW_INT_2, KW_REAL_2, KW_FLOAT, KW_NO_SPIN,
    KW_NO_CB, KW_STR_PICK };

// KW_NORMAL       Normal mode (regular spin button or entry)
// KW_INT_2        Use two integer fields (min/max)
// KW_REAL_2       Use two real fields (min/max)
// KW_FLOAT        Use exponential notation
// KW_NO_SPIN      Numeric entry but no adjustment
// KW_NO_CB        Special callbacks for debug popup
// KW_STR_PICK     String selection - callback arg passed to
//                  create_widgets()

// Class used in the keyword entry dialogs.
//
class QTent : public QGroupBox, public userEnt
{
    QTent(void(*cb)(bool, variable*, xEnt*))
    {
        update = (void(*)(bool, variable*, void*))cb;
        val = 0.0;
        del = 0.0;
        pgsize = 0.0;
        rate = 0.0;
        numd = 0;
        defstr = 0;
        active = 0;
        deflt = 0;
        entry = 0;
        entry2 = 0;
        frame = 0;
        thandle = 0;
        down = false;
        mode = KW_NORMAL;
    }

    virtual ~QTent()
    {
        delete [] defstr;
    }

    void setup(float v, float d, float p, float r, int n)
    {
        val = v;
        del = d;
        pgsize = p;
        rate = r;
        numd = n;
    }

    void callback(bool isset, variable *v)
    {
        if (update)
            (*update)(isset, v, this);
    }

    void create_widgets(sKWent<xEnt>*, const char*,
        int(*)(QWidget*, QEvent*, void*) = 0);
    void set_state(bool);
    void handler(void*);

    void (*update)(bool, variable*, void*);

    float       val;                // default numeric value
    float       del,
    float       pgsize,
    float       rate;               // spin button parameters
    int         numd;               // fraction digits shown
    const char  *defstr;            // default string
    QCheckBox   *active;            // "set" check box
    QPushButton *deflt;             // "def" button
    QLineEdit   *entry;             // entry area
    QLineEdit   *entry2;            // entry area
    QGroupBox   *frame;             // top level of composite
    int         thandle;            // timer handle
    bool        down;               // decrement flag
    EntryMode   mode;               // operating mode
};
typedef sKWent<QTent> xKWent;

//
// Some global callback functions for use as an argument to
// xEnt::xEnt(), for generic data.
//
extern void kw_bool_func(bool, variable*, xEnt*);
extern void kw_int_func(bool, variable*, xEnt*);
extern void kw_real_func(bool, variable*, xEnt*);
extern void kw_string_func(bool, variable*, xEnt*);
/*
extern int kw_float_hdlr(GtkWidget*, GdkEvent*, void*);
*/
