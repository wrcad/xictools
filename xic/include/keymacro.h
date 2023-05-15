
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

#ifndef KEYMACRO_H
#define KEYMACRO_H


//
// Structs for keyboard macros.
//

enum { KEY_PRESS = 1, KEY_RELEASE, BUTTON_PRESS, BUTTON_RELEASE };

struct sEvent
{
    sEvent()
        {
            widget_name = 0;
            state = type = 0;
            next = 0;
        }

    virtual ~sEvent() { delete [] widget_name; }
    virtual void print(FILE*) = 0;
    virtual void print(char*, int) = 0;
    virtual bool exec() = 0;

    char *widget_name;
    unsigned int state;
    unsigned int type;
    sEvent *next;
};

struct sKeyEvent : public sEvent
{
    sKeyEvent(char*, unsigned int, unsigned int, unsigned int);

    void print(FILE*);
    void print(char*, int);
    void set_text();
    bool exec();

    unsigned int key;
    char text[4];
};

struct sBtnEvent : public sEvent
{
    sBtnEvent(char*, unsigned int, unsigned int, unsigned int, int, int);

    void print(FILE*);
    void print(char*, int);
    bool exec();

    unsigned int button;
    int x, y;
};

struct sKeyMap
{
    sKeyMap(unsigned int, unsigned int, const char*);
    ~sKeyMap()
        { 
            clear_response();
            delete [] forstr;
        }

    void begin_recording(char*);
    void show();
    void print_macro(FILE*);
    void print_macro_text(FILE*);
    void add_response(sEvent *ev);
    void clear_response();
    void clear_last_event();
    void fix_modif();
    void remove(sEvent*);

    unsigned int state;
    unsigned int key;
    int lastkey;
    char text[4];
    char *forstr;
    sEvent *response;
    sEvent *end;
    void *grabber;
    sKeyMap *next;
};

inline class cKbMacro *KbMac();

class cKbMacro
{
    static cKbMacro *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cKbMacro *KbMac() { return (cKbMacro::ptr()); }

    // The macro execution queue.
    //
    struct sMqueue
    {
        sMqueue(unsigned v, unsigned s, sKeyMap *k)
            {
                value = v;
                state = s;
                kl = k;
                next = 0;
            }

        unsigned value;
        unsigned state;
        sKeyMap *kl;
        sMqueue *next;
    };

    // To deal with context switches (i.e., text input mode) we
    // implement a simple stack.
    //
    struct sContext
    {
        sContext(sEvent *ev, sContext *n)
            {
                event = ev;
                next = n;
            }

        sEvent *event;
        sContext *next;
        static bool PopContext;
    };

    cKbMacro();

    void GetMacro();
    void SaveMacro(sKeyMap*, bool);
    void MacroParse(SIfile*, stringlist**, const char**, int*);
    bool DoMacro();
    bool MacroExpand(unsigned int, unsigned int, bool);
    bool MacroPush(sEvent*);
    bool MacroPop();
    void MacroFileUpdate(const char*);
    void SprintKey(char*, int, unsigned int, unsigned int, const char*);
    void SprintBtn(char*, int, int, unsigned int);

    // These are provided by the graphics package.
    sKeyMap *getKeyToMap();
    bool isModifier(unsigned int);
    bool isControl(unsigned int);
    bool isShift(unsigned int);
    bool isAlt(unsigned int);
    char *keyText(unsigned int, unsigned int);
    void keyName(unsigned int, char*);
    bool isModifierDown();
    bool notMappable(unsigned int, unsigned int);
    bool execKey(sKeyEvent*);
    bool execBtn(sBtnEvent*);

private:
    sKeyMap *already_mapped(unsigned int, unsigned int);

    sKeyMap *km_key_list;
    void *km_last_btn;
    void *km_last_menu;
    sMqueue *km_mqueue;
    sMqueue *km_mex;
    sContext *km_context;
    bool km_context_pop;

    static cKbMacro *instancePtr;
};

#endif

