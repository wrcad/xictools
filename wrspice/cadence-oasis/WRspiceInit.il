; Initialize OASIS and load the WRspice simulator context.
asiAnalogAutoloadProc()
loadContext( prependInstallPath( "etc/context/asimenv.cxt" ) )
load( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/WRspice.ini" ) )
loadContext( strcat( getShellEnvVar("CDS_WRSPICE_HOME") "/WRspice.cxt" ) )

; Update the ToolFilter to include WRspice, enabled by default.  The
; same effect can be achieved site-wide by modifying the file
; $CDSHOME/tools/dfII/etc/tools/auCore/.cdsenv
envSetVal( "auCore.toolFilter"  "toolList"      'string "spectre ams auCdl auLvs hspiceD UltraSim aps WRspice" )
envSetVal( "auCore.toolFilter"  "defaultTools"  'string "spectre auCdl auLvs WRspice" )

; Set WRspice as the default simulator.
envSetVal( "asimenv.startup"    "simulator"     'string "WRspice" )

; Specify the location of the Jostphson junction WRspice device models.
envSetVal( "WRspice.envOpts"    "modelFiles"    'string "$CDS_WRSPICE_HOME/wrsmods/models.scs" )

; Set location for simulator files, this is the default.
;envSetVal( "asimenv.startup"   "projectDir"    'string "~/simulation")

