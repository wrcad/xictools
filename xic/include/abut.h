
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

#ifndef ABUT_H
#define ABUT_H

// The abutment events, per Ciranova.
//
enum aevType { aevAbut, aevUnAbut, aevTestAbut, aevInstAbut };

// Info for an abutable pin shape.
//
struct sAbutPinInfo
{
    sAbutPinInfo()
        {
            pi_pin_size = 0.0;
            pi_class_name = 0;
            pi_shape_name = 0;
            pi_directions = 0;
        }

    ~sAbutPinInfo()
        {
            delete [] pi_class_name;
            delete [] pi_shape_name;
            delete [] pi_directions;
        }

    double pin_size()               const { return (pi_pin_size); }
    const char *class_name()        const { return (pi_class_name); }
    const char *shape_name()        const { return (pi_shape_name); }
    const char *directions()        const { return (pi_directions); }

    sAbutPinInfo *dup() const
        {
            sAbutPinInfo *pi = new sAbutPinInfo;
            pi->pi_pin_size = pi_pin_size;
            pi->pi_class_name = lstring::copy(pi_class_name);
            pi->pi_shape_name = lstring::copy(pi_shape_name);
            pi->pi_directions = lstring::copy(pi_directions);
            return (pi);
        }

    bool setup(const CDo*);
    static bool checkDirections(const cTfmStack&, const cTfmStack&,
        const char*, const char*);

private:
    double pi_pin_size;
    char *pi_class_name;
    char *pi_shape_name;
    char *pi_directions;
};

// Keyword/value pair list element.
//
struct sAbutKeyVal
{
    sAbutKeyVal(char *k, char *v)
        {
            kv_key = k;
            kv_value = v;
            kv_next = 0;
        }

    ~sAbutKeyVal()
        {
            delete [] kv_key;
            delete [] kv_value;
        }

    static void destroy(sAbutKeyVal *v)
        {
            while (v) {
                sAbutKeyVal *vx = v;
                v = v->next();
                delete vx;
            }
        }

    const char *key()           const { return (kv_key); }
    const char *value()         const { return (kv_value); }
    sAbutKeyVal *next()               { return (kv_next); }
    void set_next(sAbutKeyVal *n)     { kv_next = n; }
    const sAbutKeyVal *nextc()  const { return (kv_next); }

private:
    char *kv_key;
    char *kv_value;
    sAbutKeyVal *kv_next;
};

// Enum corresponding to the Ciranova rule names.
//
enum aRuleType
{
    bogusValue = -1,
    noAbut = 0,
    adjSmaller,
    adjEqual,
    adjBigger,
    abut2PinSmaller,
    abut2PinEqual,
    abut2PinBigger,
    abut3PinSmaller,
    abut3PinEqual,
    abut3PinBigger
};

// Abutment rule description, parsed from the pycAbutRule/XICP_AB_RULE
// property string.
//
struct sAbutRule
{
    sAbutRule();
    ~sAbutRule();

    static bool parseRules(const char*, sAbutRule**);
    bool setup(const char*, const char*, const char*);
    void print(FILE*) const;

    static void destroy(sAbutRule *r)
        {
            while (r) {
                sAbutRule *rx = r;
                r = r->ar_next;
                delete rx;
            }
        }

    static void print_all(const sAbutRule *r, FILE *fp)
        {
            while (r) {
                r->print(fp);
                r = r->ar_next;
            }
        }

    static sAbutRule *find(sAbutRule *thisr, aRuleType n)
        {
            for (sAbutRule *r = thisr; r; r = r->next()) {
                if (r->name_val() == n)
                    return (r);
            }
            return (0);
        }

    aRuleType name_val()                const { return (ar_name); }
    double spacing()                    const { return (ar_spacing); }
    const sAbutKeyVal *moving_keys()    const { return (ar_moving_keys); }
    const sAbutKeyVal *fixed_keys()     const { return (ar_fixed_keys); }
    sAbutRule *next()                         { return (ar_next); }
    void set_next(sAbutRule *n)               { ar_next = n; }


    static const char *rule_name(int t)
        {
            return (t >= 0 && t <= abut3PinBigger ? ar_rule_names[(int)t] : 0);
        }

    aRuleType rule_type(const char *nm)
        {
            if (nm) {
                for (const char **s = ar_rule_names; *s; s++) {
                    if (!strcmp(*s, nm))
                        return ((aRuleType)(int)(s - ar_rule_names));
                }
            }
            return (bogusValue);
        }

private:
    aRuleType ar_name;              // Value of _name, as enum.
    double ar_spacing;              // Value of _spacing.
    sAbutKeyVal *ar_moving_keys;    // Key/vals of moving instance.
    sAbutKeyVal *ar_fixed_keys;     // Key/vals of fixed instance.
    sAbutRule *ar_next;

    static const char *ar_rule_names[];
};

// List element for abutment info.
struct sAbutItem
{
    sAbutItem(CDc *cd, CDo *od1, CDo *od2,
            const sAbutPinInfo &pi1, const sAbutPinInfo &pi2)
        {
            ai_next = 0;
            ai_cdesc2 = cd;
            ai_odesc1 = od1;
            ai_odesc2 = od2;
            ai_info1 = pi1.dup();
            ai_info2 = pi2.dup();
        }

    ~sAbutItem()
        {
            delete ai_info1;
            delete ai_info2;
        }

    static void destroy(sAbutItem *ai)
        {
            while (ai) {
                sAbutItem *ax = ai;
                ai = ai->ai_next;
                delete ax;
            }
        }

    sAbutItem *next()               const { return (ai_next); }
    void set_next(sAbutItem *n)     { ai_next = n; }
    CDc *cdesc2()                   const { return (ai_cdesc2); }
    void set_cdesc2(CDc *cd)        { ai_cdesc2 = cd; }
    CDo *odesc1()                   const { return (ai_odesc1); }
    CDo *odesc2()                   const { return (ai_odesc2); }
    sAbutPinInfo *info1()           const { return (ai_info1); }
    sAbutPinInfo *info2()           const { return (ai_info2); }

private:
    sAbutItem *ai_next;
    CDc *ai_cdesc2;
    CDo *ai_odesc1;
    CDo *ai_odesc2;
    sAbutPinInfo *ai_info1;
    sAbutPinInfo *ai_info2;

};

// Structure for holding reversion information.
//
struct sAbutPrior
{
    sAbutPrior(CDc *cd)
        {
            ap_self = cd;
            ap_class = 0;
            ap_self_shape = 0;
            ap_ptnr_shape = 0;
            ap_ldesc = 0;
            ap_params = 0;
            ap_id = 0;
        }

    sAbutPrior(CDc*, unsigned int, sAbutItem*, sAbutKeyVal*);

    ~sAbutPrior()
        {
            delete [] ap_class;
            delete [] ap_self_shape;
            delete [] ap_ptnr_shape;
            sAbutKeyVal::destroy(ap_params);
        }

    bool parse(const char*);
    char *string();
    CDc *findNewPartnerInList(CDol*, CDp**) const;
    CDc *findPartner(CDp**) const;
    bool updatePin();
    bool revertPartnerAbutment();

    static unsigned int newId(const CDc*, const CDc*);

    const CDc *cdesc()              const { return (ap_self); }
    const char *class_name()        const { return (ap_class); }
    const char *self_shape_name()   const { return (ap_self_shape); }
    const char *ptnr_shape_name()   const { return (ap_ptnr_shape); }
    const CDl *ldesc()              const { return (ap_ldesc); }
    const BBox *pinBB()             const { return (&ap_pinBB); }
    const sAbutKeyVal *params()     const { return (ap_params); }

    void set_id_number(unsigned int u) { ap_id = u; }
    unsigned int id_number()        const { return (ap_id); }

private:
    CDc *ap_self;               // This instance.
    char *ap_class;             // Class name of abutted pin.
    char *ap_self_shape;        // Shape name of my pin.
    char *ap_ptnr_shape;        // Shape name of partner's pin.
    CDl *ap_ldesc;              // Pin layer.
    BBox ap_pinBB;              // My pin BB in parent-cell coordinates.
    sAbutKeyVal *ap_params;     // List of original param/vals.
    unsigned int ap_id;         // ID number.
};

// Main class for abutment handling.
//
class cAbutHandler
{
public:
    cAbutHandler()
        {
            ah_cdesc1 = 0;
            ah_prms1 = 0;
            ah_list = 0;
        }

    ~cAbutHandler()
        {
            PCellParam::destroy(ah_prms1);
            sAbutItem::destroy(ah_list);
        }

    void set_params(PCellParam *p)      { ah_prms1 = p; }

    bool checkAbutment(CDc*);
    bool handleAbutment();

    static bool setNewParams(CDc*, const PCellParam*, CDc** = 0);

private:
    bool fixPosition(const sAbutItem*, const sAbutRule*, const sAbutRule*,
        CDc*, const CDc*);
    bool checkAbut(CDc*, CDc*);

    CDc *ah_cdesc1;             // First (moving) instance
    PCellParam *ah_prms1;       // Modified parameters for first instance.
    sAbutItem *ah_list;         // List of abutments.
};

// Main controller class.
//
class cAbutCtrl
{
public:
    cAbutCtrl()
        {
            ac_abut_instances = 0;
            ac_no_check = false;
        }

    void addInstance(CDc*);
    void handleAbutment();

    void setNoCheck(bool b)     { ac_no_check = b; }

private:
    CDol *ac_abut_instances;
    bool ac_no_check;
};

#endif

