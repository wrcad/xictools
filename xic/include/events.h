
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef EVENTS_H
#define EVENTS_H


// Maximum depth of nested operations.
#define CallStackDepth 5

// Device independent keycodes.
enum eKeyCode
{
    NO_KEY,
    RETURN_KEY,
    ESCAPE_KEY,
    TAB_KEY,
    UNDO_KEY,
    REDO_KEY,
    BREAK_KEY,
    DELETE_KEY,
    BSP_KEY,
    LEFT_KEY,
    UP_KEY,
    RIGHT_KEY,
    DOWN_KEY,
    SHIFTDN_KEY,
    SHIFTUP_KEY,
    CTRLDN_KEY,
    CTRLUP_KEY,
    HOME_KEY,
    NUPLUS_KEY,
    NUMINUS_KEY,
    PAGEDN_KEY,
    PAGEUP_KEY,
    FUNC_KEY
};

// Actions corresponding to keyboard input
//
enum eKeyAction
{
    No_action, Iconify_action, Interrupt_action, Escape_action,
    Redisplay_action, Delete_action, Bsp_action, CodeUndo_action,
    CodeRedo_action, Undo_action, Redo_action, Expand_action,
    Grid_action, ClearKeys_action, DRCb_action, DRCf_action,
    DRCp_action, SetNextView_action, SetPrevView_action,
    FullView_action, DecRot_action, IncRot_action, FlipY_action,
    FlipX_action, PanLeft_action, PanDown_action, PanRight_action,
    PanUp_action, ZoomIn_action, ZoomOut_action, ZoomInFine_action,
    ZoomOutFine_action, Command_action, Help_action, Coord_action,
    SaveView_action, NameView_action, Version_action,
    PanLeftFine_action, PanDownFine_action, PanRightFine_action,
    PanUpFine_action, IncExpand_action, DecExpand_action
};

// Cursor descriptor.  This keeps track of button1 press/release
// locations, and the state at the event times.
//
struct CursorDesc
{
    CursorDesc()
        {
            c_raw_x = c_raw_y = 0;
            c_x = c_y = 0;
            c_prev_x = c_prev_y = 0;
            c_ref_x = c_ref_y = 0;
            c_dx = c_dy = 0;
            c_up_x = c_up_y = 0;
            c_upstate = 0;
            c_downstate = 0;
            c_window = 0;

            c_raw_x_a = 0;
            c_raw_y_a = 0;
            c_up_x_a = 0;
            c_up_y_a = 0;
            c_window_a = 0;

            c_press_ok = false;
            c_release_ok = false;
            c_press_alt = false;
            c_was_press_alt = false;
        }

    void get_reference(int *x, int *y)  const { *x = c_ref_x; *y = c_ref_y; }

    void get_xy(int *x, int *y)         const { *x = c_x; *y = c_y; }
    void get_raw(int *x, int *y)        const { *x = c_raw_x; *y = c_raw_y; }
    void get_release(int *x, int *y)    const { *x = c_up_x; *y = c_up_y; }
    void get_prev(int *x, int *y)       const { *x = c_prev_x; *y = c_prev_y; }
    void get_dxdy(int *dx, int *dy)     const { *dx = c_dx; *dy = c_dy; }

    bool is_press_ok()                  const { return (c_press_ok); }
    bool is_release_ok()                const { return (c_release_ok); }

    int get_upstate()                   const { return (c_upstate); }
    int get_downstate()                 const { return (c_downstate); }
    unsigned long get_window()          const { return (c_window); }

    void get_alt_down(int *x, int *y) const { *x = c_raw_x_a; *y = c_raw_y_a; }
    void get_alt_up(int *x, int *y)     const { *x = c_up_x_a; *y = c_up_y_a; }
    int get_alt_downstate()             const { return (c_downstate_a); }
    int get_alt_upstate()               const { return (c_upstate_a); }
    unsigned long get_alt_window()      const { return (c_window_a); }
    bool get_press_alt()                const { return (c_press_alt); }
    bool get_was_press_alt()            const { return (c_was_press_alt); }

    void set_reference(int x, int y)    { c_ref_x = x; c_ref_y = y; }

    void press_handler(WindowDesc*, int, int, int);
    void release_handler(WindowDesc*, int, int, int);
    void noop_handler(WindowDesc*, int, int);
    void phony_press(WindowDesc*, int, int);

private:
    void set_xy_and_raw(int x, int y, int xr, int yr)
        { c_x = x; c_y = y; c_raw_x = xr; c_raw_y = yr; }

    void set_press_ok(bool b)           { c_press_ok = b; }
    void set_release_ok(bool b)         { c_release_ok = b; }

    void set_alt_down(int x, int y)     { c_raw_x_a = x; c_raw_y_a = y; }
    void set_alt_up(int x, int y)       { c_up_x_a = x; c_up_y_a = y; }
    void set_alt_downstate(int s)       { c_downstate_a = s; }
    void set_alt_upstate(int s)         { c_upstate_a = s; }
    void set_alt_window(unsigned long w){ c_window_a = w; }
    void set_press_alt(bool b)          { c_press_alt = b; }
    void set_was_press_alt(bool b)      { c_was_press_alt = b; }

    void update_down(unsigned long winid, int xr, int yr, int xg, int yg,
        int state)
        {
            c_window = winid;
            c_prev_x = c_x;
            c_prev_y = c_y;
            c_raw_x = xr;
            c_raw_y = yr;
            c_x = xg;
            c_y = yg;
            c_ref_x = c_x;
            c_ref_y = c_y;
            c_dx = c_x - c_prev_x;
            c_dy = c_y - c_prev_y;
            c_downstate = state;
        }

    void update_up(int xr, int yr, int state)
        {
            c_up_x = xr;
            c_up_y = yr;
            c_upstate = state;
        }

    int c_raw_x, c_raw_y;       // Last accepted press location.
    int c_x, c_y;               // Last accepted press location, snapped.
    int c_prev_x, c_prev_y;     // Previous accepted location.
    int c_ref_x, c_ref_y;       // Reference location, for measurement only.
    int c_dx, c_dy;             // Diff x, prev_x and y, prev_y.
    int c_up_x, c_up_y;         // Release location.
    int c_upstate;              // Modifier state at press.
    int c_downstate;            // Modifier state at release.
    unsigned long c_window;     // Window of press.

    // Save locations for press/release in non-similar window.
    int c_raw_x_a, c_raw_y_a;   // Down locs for alt window.
    int c_up_x_a, c_up_y_a;     // Up locs for alt window.
    int c_upstate_a;            // Modifier state at press.
    int c_downstate_a;          // Modifier state at release.
    unsigned long c_window_a;   // Alternate window number of press.

    bool c_press_ok;            // Press within window.
    bool c_release_ok;          // Release within window.
    bool c_press_alt;           // Press was in alt window.
    bool c_was_press_alt;       // Press was in alt window, but release was
                                // in another window.
};

// Callback funcs for commands.  This is subclassed by button commands
// that need to filter pointer and keyboard events.  When the command
// is active, a pointer to the subclass is pushed on to the call stack.
//
struct CmdState
{
    CmdState(const char *sname, const char *hkey)
        {
            // Passed strings are not copied or freed!
            StateName = sname;
            HelpKey = hkey;
            Level = 0;
        }
    virtual ~CmdState() { }

    virtual void b1down() { }
    virtual void b1up() { }
    virtual void desel() { }
    virtual void esc() { }
    virtual bool key(int, const char*, int) { return (false); }
    virtual void b1down_altw() { }
    virtual void b1up_altw() { }

    virtual void undo();
    virtual void redo();
    
    virtual bool check_similar(const WindowDesc*, const WindowDesc*)
        { return (false); }

    const char *Name()                      { return (StateName); }
    const char *HelpKword()                 { return (HelpKey); }
    int CmdLevel()                          { return (Level); }

    static void SetAbort(bool b)            { Abort = b; }
    static void SetEnableExport(bool b)     { EnableExport = b; }
    static void SetExported(bool b)         { Exported = b; }

protected:
    const char *StateName;      // Name for command or state.
    const char *HelpKey;        // Help system keyword.
    int Level;                  // Internal state variable.  If nonzero, the
                                // command is 'active'.

    static bool Abort;          // Set when another command is starting, and
                                // this command should return immediately when
                                // the esc method is called.

    static bool EnableExport;   // Set to enable coordinate passing to the
                                // next command.
    static bool Exported;       // The exiting command passed a coordinate.
    static int ExportX;         // The X coordinate.
    static int ExportY;         // The Y coordinate.
};

// This can be instanttiated in a command top-level function, and will
// deselect the caller button when the function exits.
//
struct Deselector
{
    Deselector(CmdDesc*);       // Armed when created with non-null cmd.
    ~Deselector();              // Deselects cmd->caller.

    void clear() { d_cmd = 0; } // Disarm.

private:
    CmdDesc *d_cmd;
};


inline class cEventHdlr *EV();

// Event handler interface.
//
class cEventHdlr
{
    static cEventHdlr *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cEventHdlr *EV() { return (cEventHdlr::ptr()); }

    cEventHdlr();
    void DownTimer(int);
    bool UpTimer();
    void RegisterMainState(CmdState*);
    bool PushCallback(CmdState*);
    void PopCallback(CmdState*);
    void InitCallback();
    bool IsCallback();
    void ResizeCallback(WindowDesc*, int, int);
    bool KeypressCallback(WindowDesc*, int, char*, int);
    bool KeyActions(WindowDesc*, eKeyAction, int*);
    void PanPress(WindowDesc*, int, int, int);
    void PanRelease(WindowDesc*, int, int, int);
    void ZoomPress(WindowDesc*, int, int, int);
    void ZoomRelease(WindowDesc*, int, int, int);
    void Button1Callback(WindowDesc*, int, int, int);
    void Button1ReleaseCallback(WindowDesc*, int, int, int);
    void Button2Callback(WindowDesc*, int, int, int);
    void Button2ReleaseCallback(WindowDesc*, int, int, int);
    void Button3Callback(WindowDesc*, int, int, int);
    void Button3ReleaseCallback(WindowDesc*, int, int, int);
    void ButtonNopCallback(WindowDesc*, int, int, int);
    void ButtonNopReleaseCallback(WindowDesc*, int, int, int);
    void MotionCallback(WindowDesc*, int);
    void MotionClear();
    bool ProcessModState(int);
    void PanicPrint(FILE*);
    WindowDesc *ButtonWin(bool = false);

    WindowDesc *CurrentWin()            { return (ev_current_win); }
    WindowDesc *ZoomWin()               { return (ev_zoom_win); }
    WindowDesc *KeypressWin()           { return (ev_keypress_win); }
    void SetCurrentWin(WindowDesc *w)   { ev_current_win = w; }
    void SetKeypressWin(WindowDesc *w)  { ev_keypress_win = w; }
    void FinishZoom()                   { ev_zoom_win = 0; }
    CmdState *CurCmd()                  { return (ev_callbacks[0]); }
    CmdState *MainCmd()                 { return (ev_main_cmd); }

    void SetMotionTask(void(*task)())   { ev_motion_task = task; }

    void SetReference(int x, int y)     { ev_cursor_desc.set_reference(x, y); }
    void GetReference(int *x, int *y)   { ev_cursor_desc.get_reference(x, y); }

    void SetInCoordEntry(bool b)        { ev_in_coord_entry = b; }
    bool InCoordEntry()                 { return (ev_in_coord_entry); }
    void SetMainStateReady(bool b)      { ev_main_state_ready = b; }
    bool MainStateReady()               { return (ev_main_state_ready); }
    void SetConstrained(bool b)         { ev_constrained = b; }
    bool IsConstrained()                { return (ev_constrained); }

    const CursorDesc &Cursor()          { return (ev_cursor_desc); }

    bool IsFromPromptLine()             { return (ev_from_prline); }
    void SetFromPromptLine(bool b)      { ev_from_prline = b; }

    // Basic selection handling functions for export.
    static void sel_b1down();

// Special argument to  sel_b1up, sel_b1up_altw.
#define B1UP_NOSEL (CDol**)1

    static bool sel_b1up(BBox*, const char*, CDol**, bool = false);
    static void sel_b1down_altw();
    static bool sel_b1up_altw(BBox*, const char*, CDol**, int*, bool = false);
    static void sel_esc();
    static bool sel_undo();
    static void sel_redo(bool = false);

private:
    static int timeout(void*);

    CmdState *ev_callbacks[CallStackDepth]; // Callback stack.
    WindowDesc *ev_current_win;     // Drawing window of last non-key event.
    WindowDesc *ev_keypress_win;    // Drawing window of last key press.
    WindowDesc *ev_zoom_win;        // Set during zoom, needed for ghosting
    CmdState *ev_main_cmd;          // Actual main command.

    void(*ev_motion_task)();        // Something to do while pointer moves.

    bool ev_in_coord_entry;         // Keyboard coord entry started.
    bool ev_main_state_ready;       // Main command state is ready.
    bool ev_constrained;            // Flag: motion constraint applied.
    bool ev_shift_down;             // Flag: Shift is down
    bool ev_ctrl_down;              // Flag: Control is down
    bool ev_alt_down;               // Flag: Alt is down
    bool ev_from_prline;            // Flag: mouse pointer was in prompt area
                                    // at last key press

    CursorDesc ev_cursor_desc;      // Cursor data.

    // Used by basic selection callbacks.
    static struct selstate_t
    {
        int state;
        int id;
        bool ghost_on;
        BBox AOI;
    } selstate;

    static cEventHdlr *instancePtr;
};

#endif
