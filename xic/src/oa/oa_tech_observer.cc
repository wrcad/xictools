
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
 $Id: oa_tech_observer.cc,v 1.5 2012/12/03 00:17:45 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "oa.h"
#include "oa_tech_observer.h"


// Global Instantiation
//
oaString
cOAtechObserver::format(
    "ERROR: A %s conflict has been detected in the technology "
    "hierarchy. It is caused by the following list of %s: %s");

oaNativeNS cOAtechObserver::ns;


// This is the default constructor for the cOAtechObserver class.
//
cOAtechObserver::cOAtechObserver(oaUInt4 priorityIn) :
    oaObserver<oaTech>(priorityIn)
{
}


// This is the observer function definition for tech conflict.  It
// handles conflicts by calling
// oaIncTechConflictTest::handleConflicts().
//
void
cOAtechObserver::onConflict(oaTech*,
    oaTechConflictTypeEnum conflictType, oaObjectArray conflictingObjs)
{
    oaString            str;
    oaString            list;
    oaLayer             *layer;
    oaDerivedLayer      *derivedLay;
    oaPurpose           *purpose;
    oaSiteDef           *siteDef;
    oaViaDef            *viaDef;
    oaViaSpec           *viaSpec;
    oaAnalysisLib       *analysisLib;
    oaConstraint        *constraint;
    oaConstraintGroup   *constraintGroup;
    oaViaVariant        *viaVariant;

    for (oaUInt4 i = 0; i < conflictingObjs.getNumElements(); i++) {
        oaObject *obj = conflictingObjs[i];

        switch (conflictType) {
        case oacLayerNameTechConflictType:
        case oacLayerNumTechConflictType:
            layer = (oaLayer*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (layer->getName(str), str);
            list += (str.format(" (#%d); ", layer->getNumber()), str);
            break;

        case oacDerivedLayerTechConflictType:
            derivedLay = (oaDerivedLayer*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (derivedLay->getName(str), str) + " (";

            if ((layer = derivedLay->getLayer1()) != 0)
                list += (layer->getName(str), str) + ", ";
            else
                list += (str.format("#%d, ", derivedLay->getLayer1Num()), str);

            if ((layer = derivedLay->getLayer2()) != 0)
                list += (layer->getName(str), str) + "); ";
            else
                list += (str.format("#%d); ", derivedLay->getLayer2Num()), str);

            break;

        case oacPurposeNameTechConflictType:
        case oacPurposeNumTechConflictType:
            purpose = (oaPurpose*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (purpose->getName(str), str);
            list += (str.format(" (#%d); ", purpose->getNumber()), str);
            break;

        case oacSiteDefNameTechConflictType:
            siteDef = (oaSiteDef*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (siteDef->getName(str), str) + "; ";
            break;

        case oacViaDefNameTechConflictType:
            viaDef = (oaViaDef*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (viaDef->getName(str), str) + "; ";
            break;

        case oacViaSpecTechConflictType:
            viaSpec = (oaViaSpec*) obj;
            list += (getObjectLibName(obj, str), str) + "/(";

            if ((layer = viaSpec->getLayer1()) != 0)
                list += (layer->getName(str), str) + ", ";
            else
                list += (str.format("#%d, ", viaSpec->getLayer1Num()), str);

            if ((layer = viaSpec->getLayer2()) != 0)
                list += (layer->getName(str), str) + "); ";
            else
                list += (str.format("#%d); ", viaSpec->getLayer2Num()), str);

            break;

        case oacAnalysisLibNameTechConflictType:
            analysisLib = (oaAnalysisLib*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (analysisLib->getName(str), str) + "; ";
            break;

        case oacConstraintNameTechConflictType:
            constraint = (oaConstraint*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (constraint->getName(str), str) + "; ";
            break;

        case oacConstraintGroupNameTechConflictType:
            constraintGroup = (oaConstraintGroup*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (constraintGroup->getName(str), str) + "; ";
            break;

        case oacViaVariantNameTechConflictType:
        case oacStdViaVariantTechConflictType:
        case oacCustomViaVariantTechConflictType:
            viaVariant = (oaViaVariant*) obj;
            list += (getObjectLibName(obj, str), str) + "/";
            list += (viaVariant->getName(str), str) + "; ";
            break;

        default:
            break;
        }
    }

    oaString type;
    oaString elements;
    switch (conflictType) {
    case oacLayerNameTechConflictType:
        type = "Layer Name";
        elements = "layers";
        break;

    case oacLayerNumTechConflictType:
        type = "Layer Number";
        elements = "layers";
        break;

    case oacDerivedLayerTechConflictType:
        type = "Derived Layer";
        elements = "derived layers";
        break;

    case oacPurposeNameTechConflictType:
        type = "Layer Purpose Name";
        elements = "purposes";
        break;

    case oacPurposeNumTechConflictType:
        type = "Layer Purpose Number";
        elements = "purposes";
        break;

    case oacSiteDefNameTechConflictType:
        type = "Site Definition";
        elements = "sites";
        break;

    case oacViaDefNameTechConflictType:
        type = "Via Def Name";
        elements = "via definitions";
        break;

    case oacViaSpecTechConflictType:
        type = "Via Spec";
        elements = "via specifications";
        break;

    case oacAnalysisLibNameTechConflictType:
        type = "Analysis Library Name";
        elements = "analysis libraries";
        break;

    case oacConstraintNameTechConflictType:
        type = "Constraint Name";
        elements = "constraints";
        break;

    case oacConstraintGroupNameTechConflictType:
        type = "Constraint Group Name";
        elements = "constraint groups";
        break;

    case oacViaVariantNameTechConflictType:
        type = "Via Variant Name";
        elements = "via variants";
        break;

    case oacStdViaVariantTechConflictType:
        type = "Standard Via Variant";
        elements = "standard via variants";
        break;

    case oacCustomViaVariantTechConflictType:
        type = "Custom Via Variants";
        elements = "custom via variants";
        break;

    default:
        type = "Unknown";
        elements = "Unknown";
        break;
    }

    emitErrorMsg(type, elements, list);
}


// This function handles clearance measure conflict between tech libs.
//
void
cOAtechObserver::onClearanceMeasureConflict(oaTechArray cTechs)
{
    oaString list;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";
        list += cTechs[i]->getClearanceMeasure(true).getName() + "); ";
    }

    emitErrorMsg("Clearance Measure", "Libraries", list);
}


// This function handles manufacturing grid conflicts.
//
void
cOAtechObserver::onDefaultManufacturingGridConflict(oaTechArray cTechs)
{
    oaString list;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";
        str.format("%d); ", cTechs[i]->getDefaultManufacturingGrid(true));
        list += str;
    }

    emitErrorMsg("Default Manufacturing Grid", "Libraries", list); }


// This function handles conflicts between tech libs related to gate
// grounding.
//
void
cOAtechObserver::onGateGroundedConflict(oaTechArray cTechs)
{
    oaString list;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";

        if (cTechs[i]->isGateGrounded(true))
            list += "grounded); ";
        else
            list += "not grounded); ";
    }

    emitErrorMsg("Gate Grounded", "Libraries", list);
}


// This function handles conflicts between tech libs related to DBU
// per UU.
//
void
cOAtechObserver::onDBUPerUUConflict(oaTechArray cTechs, oaViewType *viewType)
{
    oaString list;
    oaString viewTypeName;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";
        str.format("%d); ", cTechs[i]->getDBUPerUU(viewType, true));
        list += str;
    }

    viewType->getName(viewTypeName);
    emitErrorMsg("DBU per UU (" + viewTypeName + ")", "Libraries", list);
}


// This function handles conflicts between tech libs related to user
// units.
//
void
cOAtechObserver::onUserUnitsConflict(oaTechArray cTechs, oaViewType *viewType)
{
    oaString list;
    oaString viewTypeName;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";
        list += cTechs[i]->getUserUnits(viewType, true).getName() + "); ";
    }

    viewType->getName(viewTypeName);
    emitErrorMsg("User Units (" + viewTypeName + ")", "Libraries", list);
}


// This function handles conflicting process family attributes.
//
void
cOAtechObserver::onProcessFamilyConflict(const oaTechArray &cTechs)
{
    oaString list;
    oaString str;
    for (oaUInt4 i = 0; i < cTechs.getNumElements(); i++) {
        list += (getObjectLibName(cTechs[i], str), str) + " (";
        list += (cTechs[i]->getProcessFamily(str, true), str) + "); ";
    }

    emitErrorMsg("Process Family", "Libraries", list);
}


// This function handles conflicting layer definitions.
//
void
cOAtechObserver::onExcludedLayerConflict(oaTech*,
    const oaPhysicalLayer *l1, const oaLayer *l2)
{
    oaString    str;
    oaString    list;
    list += (getObjectLibName(l1, str), str) + "/";
    list += (l1->getName(str), str) + " is mutually exclusive with ";
    list += (getObjectLibName(l2, str), str) + "/";
    list += (l2->getName(str), str) + "; ";

    emitErrorMsg("Excluded Layer", "Layers", list);
}


// This function returns the name of the library a certain object
// resides in.
//
void
cOAtechObserver::getObjectLibName(const oaObject *obj, oaString &str) const
{
    oaTech *tech = (oaTech*)obj->getDatabase();

    // Return the name of the library that contains the techDB.
    // Alternatively, the full path to the library could be found as follows:
    //  tech->getLib()->getFullPath(str);
    //
    tech->getLibName(ns, str);
}


// This function populates a format string for a tech conflict error
// message and prints it to stdout.
//
void
cOAtechObserver::emitErrorMsg(const char *type, const char *elements,
    const char *list) const
{
    oaString str;
    str.format(format, type, elements, list);
    Errs()->add_error((const char*)str);
}

