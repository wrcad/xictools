
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: oa_tech_observer.h,v 1.3 2012/12/02 21:59:39 stevew Exp $
 *========================================================================*/

#ifndef OA_TECH_OBSERVER_H
#define OA_TECH_OBSERVER_H


// Handle technology conflict notifications.
//
class cOAtechObserver : public oaObserver<oaTech>
{
public:
    cOAtechObserver(oaUInt4 priorityIn);

    virtual void onConflict(oaTech*, oaTechConflictTypeEnum, oaObjectArray);
    virtual void onClearanceMeasureConflict(oaTechArray);
    virtual void onDefaultManufacturingGridConflict(oaTechArray);
    virtual void onGateGroundedConflict(oaTechArray);
    virtual void onDBUPerUUConflict(oaTechArray, oaViewType*);
    virtual void onUserUnitsConflict(oaTechArray, oaViewType*);
    virtual void onProcessFamilyConflict(const oaTechArray&);
    virtual void onExcludedLayerConflict(oaTech*, const oaPhysicalLayer*,
        const oaLayer*);

private:
    void getObjectLibName(const oaObject*, oaString&) const;
    void emitErrorMsg(const char *, const char*, const char*) const;

    static oaNativeNS       ns;
    static oaString         format;
};

#endif

