;
;                             Copyright (C) 1992
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/classes.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

(defclass WRspice ( asiAnalog ) nil )
(defclass WRspice_session (asiAnalog_session) nil )

(defclass WRspice_tran_analysis ( asiAnalog_analysis ) nil )
(defclass WRspice_ac_analysis ( asiAnalog_analysis ) nil )
(defclass WRspice_dc_analysis ( asiAnalog_analysis ) nil )
(defclass WRspice_noise_analysis ( asiAnalog_analysis ) nil )

