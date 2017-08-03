
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef KEYMAP_H
#define KEYMAP_H

// Element for mapping keysyms to internal code.
//
struct keymap
{
    unsigned int keyval;
    KEYcode code;
    int subcode;
};

// Keypress action table element.
//
struct keyaction
{
    int code;
    unsigned int state;
    KeyAction action;
};

struct sKsMapElt
{
    sKsMapElt()
        {
            name = 0;
            code = 0;
            keysym = 0;
        }

    sKsMapElt(const char *n, int c, unsigned int k)
        {
            name = n;
            code = c;
            keysym = k;
        }

    const char *name;
    int code;
    unsigned int keysym;
};


inline class cKsMap *Kmap();

class cKsMap
{
    static cKsMap *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cKsMap *Kmap() { return (cKsMap::ptr()); }

    cKsMap();

    // key mapping
    bool SaveMapFile(const char*);
    bool DoMap(unsigned int);
    char *ReadMapFile(const char*);

    // mouse button mapping
    int ButtonMap(int);
    void SetButtonMap(int, int);

    void SetMap(int, unsigned int);
    char *KeyvalToString(unsigned int);
    unsigned int StringToKeyval(const char*);

    const char *GetMapName(unsigned int i)
        {
            if (i < 6)
                return (kmKeyTab[i].name);
            return (0);
        }

    int GetMapCode(unsigned int i)
        {
            if (i < 6)
                return (kmKeyTab[i].code);
            return (0);
        }

    unsigned int GetMapKeysym(unsigned int i)
        {
            if (i < 6)
                return (kmKeyTab[i].keysym);
            return (0);
        }

    void SetMapKeysym(unsigned int i, unsigned int k)
        {
            if (i < 6)
                kmKeyTab[i].keysym = k;
        }

    int SuppressChar()              { return (kmSuppressChar); }
    unsigned int KeyTab(int i)      { return (kmKeyTab[i].keysym); }
    keymap *KeymapDown()            { return (kmKeyMapDn); }
    keymap *KeymapUp()              { return (kmKeyMapUp); }
    keyaction *ActionMapPre()       { return (kmActionsPre); }
    keyaction *ActionMapPost()      { return (kmActionsPost); }

private:
    // provided by the graphics package
    void init();
    int filter_key(unsigned int);

    sKsMapElt kmKeyTab[6];
    keymap *kmKeyMapDn;
    keymap *kmKeyMapUp;
    keyaction *kmActionsPre;
    keyaction *kmActionsPost;
    int kmSuppressChar;

    static cKsMap *instancePtr;
};

#endif

