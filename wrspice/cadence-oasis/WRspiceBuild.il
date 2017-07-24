;
;                             Copyright (C) 1993
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/spice3Build.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

; This is required to build your simulator context.  e.g. here for
; WRspice.cxt build.  Going IC 5.0 onwards the customer needs to
; define a new function getContext() for this.

(if system.P64 then
	cxtDeliveryPath   = "tools/dfII/etc/context/64bit"
else
	cxtDeliveryPath   = "tools/dfII/etc/context"
)

(defun getContext (cxt)
 (let (filePath 
       (baseCxtPath (strcat (cdsGetInstPath cxtDeliveryPath) "/" cxt ".cxt")) 
       (sklCxtPath  (strcat (cdsGetInstPath "tools") "/" getSkillVersion() 
        "/context/" cxt ".cxt"))
       )
       if(isFile(baseCxtPath) then filePath = baseCxtPath
       else if(isFile(sklCxtPath) then filePath = sklCxtPath
             else printf("Could not locate file for context %s\n" cxt)))
       (cond 
           ((null (loadContext filePath))
                       (printf "load of context %s failed\n" cxt))
           ((null (callInitProc cxt))
                       (printf "init proc of context %s failed\n" cxt))
           (t (printf "Loading context %s\n" cxt))
       )
  )
)
getContext( "skillCore" )
getContext( "auCore" )
getContext( "oasis" )
getContext( "analog" )
getContext( "asimenv" )
setContext( "WRspice" )

load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/startup.il" ) )
saveContext( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/WRspice.cxt" ) )
exit()

