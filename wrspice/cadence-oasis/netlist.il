;
;                             Copyright (C) 1993
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/netlist.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

;------------------------------------------------------------------------------
;This file 
;   a) Defines the class WRspiceFormatter.
;   b) Defines the method asiInitFormatterClass.
;   c) Defines the function WRspice.
;   d) Overloads various netlister methods, most of which are provided
;   mainly to illustrate the use of these methods. They use callNextMethod
;   to get the functionality of the parent class (the analogFormatter class,
;   in this case), and then do whatever else that is required - here most
;   of the methods just print comments.
;------------------------------------------------------------------------------

(defclass WRspiceFormatter (nlAnalogFormatter) ())

(defmethod asiInitFormatterClass ( (tool WRspice) )
  (asiSetNetlistFormatterClass tool 'WRspiceFormatter )
  )

(defun WRspice () t)

(defmethod nlInitialize ( (formatter WRspiceFormatter) )
  (callNextMethod)  
  (nlSetOption (nlGetNetlister formatter) 'suffixMap (list (list "M" "Meg" )))
  (nlSetOption (nlGetNetlister formatter) 'subcktInstPrefix "X" )
  (nlSetOption (nlGetNetlister formatter) 'begComment "*")
  formatter)

(defmethod nlPrintFooter ( (formatter WRspiceFormatter) )
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter) 
;		  "--- nlPrintFooter ---\n" )
  )

; Don't print .GLOBAL without global nets.
(defmethod nlPrintHeader ( (formatter WRspiceFormatter) )
    let( ((glbout nil) (netlister nlGetNetlister(formatter)))
        nlPrintString( netlister (nlGetOption netlister 'begComment))
        nlPrintString( netlister "\n")

        foreach( glob
            setof( x (nlGetGlobalNets netlister) (nequal x "gnd!"))

	    if( (glbout == nil) then
                glbout = t
                nlPrintString( netlister ".GLOBAL")
            )
            nlPrintString(netlister " ")
            nlPrintString( netlister (nlMapGlobalNet netlister glob))
        )

        nlPrintHeaderComments( formatter)
        nlPrintString(netlister "\n")
        t
    )
)

;(defmethod nlPrintHeader ( (formatter WRspiceFormatter) )
;  (nlPrintComment (nlGetNetlister formatter) 
;		  "--- nlPrintHeader ---\n" )
;  (callNextMethod)
;    t)

(defmethod nlPrintSubcktHeaderComments ( (formatter WRspiceFormatter) _cv )
;  (nlPrintComment (nlGetNetlister formatter) 
;		  "--- nlPrintSubcktHeaderComments ---\n" )
  (callNextMethod)
  )

(defmethod nlPrintTopCellHeaderComments ( (formatter WRspiceFormatter) _cv)
;  (nlPrintComment (nlGetNetlister formatter) 
;		  "--- nlPrintTopCellHeaderComments ---\n" )
  (callNextMethod)
  )

(defmethod nlPrintTopCellFooterComments ( (formatter WRspiceFormatter) _cv)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintTopCellFooterComments ---\n" )
  )

(defmethod nlPrintTopCellHeader ( (formatter WRspiceFormatter) _cv )
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintTopCellHeader ---\n" )
  (callNextMethod)
  )

(defmethod nlPrintTopCellFooter ( (formatter WRspiceFormatter) _cv )
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintTopCellFooter ---\n" )
  )
  
(defmethod nlPrintSubcktHeader ( (formatter WRspiceFormatter) _cv )
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktHeader ---\n" )
  (callNextMethod)
  )

(defmethod nlPrintSubcktFooter ( (formatter WRspiceFormatter) _cv)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktFooter ---\n" )
  )

(defmethod nlPrintSubcktFooterComments ( (formatter WRspiceFormatter) _cv )
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktFooterComments ---\n" )
  )

(defmethod nlPrintInstComments ( (formatter WRspiceFormatter) _inst)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintInstComments ---\n" )
  )

(defmethod nlPrintInstEnd ( (formatter WRspiceFormatter) _inst)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintInstEnd ---\n" )
  )

(defmethod nlPrintSubcktBegin ( (formatter WRspiceFormatter) _cv)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktBegin ---\n" )
  (callNextMethod)
  t)

(defmethod nlPrintSubcktEnd ( (formatter WRspiceFormatter) _cv)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktEnd ---\n")
  )

(defmethod nlPrintHeaderComments ( (formatter WRspiceFormatter) )
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintHeaderComments ---\n" )
  )

(defmethod nlPrintSubcktParameters ( (formatter WRspiceFormatter) _cv)
  (callNextMethod)
;  (nlPrintComment (nlGetNetlister formatter)
;		  "--- nlPrintSubcktParameters ---\n" )
  )

