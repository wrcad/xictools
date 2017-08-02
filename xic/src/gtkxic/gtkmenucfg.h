
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
 
#ifndef GTKMENUCFG_H
#define GTKMENUCFG_H


inline class gtkMenuConfig *gtkCfg();

class gtkMenuConfig
{
public:
    friend inline gtkMenuConfig *gtkCfg() { return (gtkMenuConfig::ptr()); }

    gtkMenuConfig();
    void instantiateMainMenus();
    void instantiateTopButtonMenu();
    void instantiateSideButtonMenus();
    void instantiateSubwMenus(int, GtkItemFactory*);
    void updateDynamicMenus();
    void switch_menu_mode(DisplayMode, int);
    void set_main_global_sens(bool);

private:
    static gtkMenuConfig *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }
  
    static void on_null_ptr();

    static void menu_handler(GtkWidget*, void*, unsigned);
    static int user_cmd_proc(void*);
    static int cmd_proc(void*);
    static void make_entries(GtkItemFactory*, GtkItemFactoryEntry*, int,
        MenuEnt*, int);
    static void edmenu_proc(GtkWidget*, void*);
    static void vimenu_proc(GtkWidget*, void*);
    static void stmenu_proc(GtkWidget*, void*);
    static void shmenu_proc(GtkWidget*, void*);
    static void top_btnmenu_callback(GtkWidget*, void*);
    static void btnmenu_callback(GtkWidget*, void*);
    static int popup_btn_proc(GtkWidget*, GdkEvent*, void*);
    static const char **get_style_pixmap();

    static gtkMenuConfig *instancePtr;
};

#endif

