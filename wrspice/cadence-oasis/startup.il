;
;                             Copyright (C) 1993
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/startup.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/classes.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/initialize.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/netlist.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/WRspice.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/simControl.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/simOption.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/analysis.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/advTool.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/dataAccess.il" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/envOption.il" ) )

