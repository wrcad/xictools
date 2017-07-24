;
;                             Copyright (C) 2003
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
; $Source: /cvsrep/group/artist/src/spice3/dataAccess.il,v $
; $Author: p4admin $
; $Revision: #1 $
; $Date: 2014/12/01 $
; $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

;; For WRspice data access part file

(defmethod asiInitDataAccessFunction ( ( tool WRspice ) )
     asiDefineDataAccessFunction( tool 'VT   'WRspiceVT)
     asiDefineDataAccessFunction( tool 'VF   'WRspiceVF)
)

;;; Replace 'tran by  value of type field of
;;; transient analysis entry in the logFile e.g. "analysisTran1-tran"

procedure( WRspiceVT(specifier dataDir simData)
     asiGetDrlData('tran specifier dataDir)
)
;;; Replace 'ac by  value of type field of
;;; ac analysis entry in the logFile e.g. here "analysisAC1-ac"

procedure( WRspiceVF(specifier dataDir simData)
    asiGetDrlData('ac specifier dataDir)
)
