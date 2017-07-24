; Support for WRspice
; Whiteley Research Inc. (http://wrcad.com)
; $Id:$

; Define the WRspice options.

defmethod( asiInitSimOption (( tool WRspice ))

    asiChangeSimOptionFormProperties( tool
        ?type 'multiPage
        ?groupType 'tabs
        ?pageList  list( list("General" 'twoD) list("Timestep", 'matrix 2)
          list("Tolerance" 'matrix 2) list("Devices" 'matrix, 2))
    )

    ; General

    asiAddSimOption( tool 
        ?name       'maxdata
        ?type       'string
        ?value      "256000"
        ?min        1000
        ?page       "General"
        ?row        1
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'extprec
        ?type       'boolean
        ?page       "General"
        ?row        1
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'noklu
        ?type       'boolean
        ?page       "General"
        ?row        1
        ?column     3
    )

    asiAddSimOption( tool 
        ?name       'fpemode
        ?type       'cyclic
        ?value      "0"
        ?choices    list("0" "1" "2")  
        ?page       "General"
        ?row        2
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'savecurrent
        ?type       'boolean
        ?page       "General"
        ?row        2
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'nomatsort
        ?type       'boolean
        ?page       "General"
        ?row        2
        ?column     3
    )

    asiAddSimOption( tool 
        ?name       'loadthrds
        ?type       'string
        ?page       "General"
        ?value      "0"
        ?min        0
        ?max        31
        ?row        3
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'loopthrds
        ?type       'string
        ?page       "General"
        ?value      "0"
        ?min        0
        ?max        31
        ?row        3
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'dcoddstep
        ?type       'boolean
        ?page       "General"
        ?row        3
        ?column     3
    )

    asiAddSimOption( tool 
        ?name       'optmerge
        ?type       'cyclic
        ?value      "global"
        ?choices    list("global" "local" "noshell")  
        ?page       "General"
        ?row        4
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'parhier
        ?type       'cyclic
        ?value      "global"
        ?choices    list("global" "local")  
        ?page       "General"
        ?row        4
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'hspice
        ?type       'boolean
        ?page       "General"
        ?row        4
        ?column     3
    )

    asiAddSimOption( tool 
        ?name       'temp
        ?type       'string
        ?value      "25.0"
        ?min        -273.15
        ?max        1000.0
        ?page       "General"
        ?row        5
        ?column     1
        ?genericName 'temperature
    )

    ; Timestep
    ; For matrix, the row/columns given below are ignored.

    asiAddSimOption( tool 
        ?name       'steptype
        ?type       'cyclic
        ?choices    list("interpolate" "hitusertp" "nousertp")  
        ?value      "interpolate"
        ?page       "Timestep"
        ?row        1
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'interplev
        ?type       'cyclic
        ?choices    list("1" "2" "3" "4" "5" "6" "7" "8" "9")  
        ?value      "1"
        ?page       "Timestep"
        ?row        1
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'method
        ?type       'cyclic
        ?choices    list("trap" "gear")
        ?value      "trap"
        ?page       "Timestep"
        ?row        2
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'maxord
        ?type       'cyclic
        ?choices    list("1" "2" "3" "4" "5" "6")
        ?value      "2"
        ?page       "Timestep"
        ?row        2
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'trapratio
        ?type       'string
        ?value      "10.0"
        ?min        2.0
        ?max        100.0
        ?page       "Timestep"
        ?row        3
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'trapcheck
        ?type       'boolean
        ?page       "Timestep"
        ?row        3
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'xmu
        ?type       'string
        ?value      "0.5"
        ?min        0.0
        ?max        0.5
        ?page       "Timestep"
        ?row        4
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'spice3
        ?type       'boolean
        ?page       "Timestep"
        ?row        4
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'trtol
        ?type       'string
        ?value      "7.0"
        ?min        1.0
        ?max        100.0
        ?page       "Timestep"
        ?row        5
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'chgtol
        ?type       'string
        ?value      "1.0e-14"
        ?min        1.0e-16
        ?max        1.0e-12
        ?page       "Timestep"
        ?row        5
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'delmin
        ?type       'string
        ?value      "0.0"
        ?page       "Timestep"
        ?row        6
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'minbreak
        ?type       'string
        ?value      "0.0"
        ?page       "Timestep"
        ?row        6
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'dphimax
        ?type       'string
        ?value      "0.628"
        ?min        0.003
        ?max        3.142
        ?page       "Timestep"
        ?row        7
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'jjaccel
        ?type       'boolean
        ?page       "Timestep"
        ?row        7
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'nojjtp
        ?type       'boolean
        ?page       "Timestep"
        ?row        8
        ?column     1
    )

    ; Tolerance

    asiAddSimOption( tool 
        ?name       'abstol
        ?type       'string
        ?value      "1.0e-12"
        ?min        1.0e-15
        ?max        1.0e-9
        ?page       "Tolerance"
        ?row        1
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'reltol
        ?type       'string
        ?value      "1.0e-3"
        ?min        1.0e-8
        ?max        1.0e-2
        ?page       "Tolerance"
        ?row        1
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'vntol
        ?type       'string
        ?value      "1.0e-6"
        ?min        1.0e-9
        ?max        1.0e-3
        ?page       "Tolerance"
        ?row        2
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'pivrel
        ?type       'string
        ?value      "1.0e-3"
        ?min        1.0e-5
        ?max        1.0
        ?page       "Tolerance"
        ?row        2
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'gmin
        ?type       'string
        ?value      "1.0e-12"
        ?min        1.0e-15
        ?max        1.0e-6
        ?page       "Tolerance"
        ?row        3
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'pivtol
        ?type       'string
        ?value      "1.0e-13"
        ?min        1.0e-18
        ?max        1.0e-9
        ?page       "Tolerance"
        ?row        3
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'gminsteps
        ?type       'cyclic
        ?choices    list("-1" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9"
          "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20")
        ?value      "0"
        ?page       "Tolerance"
        ?row        1
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'srcsteps
        ?type       'cyclic
        ?choices    list("-1" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9"
          "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20")
        ?value      "0"
        ?page       "Tolerance"
        ?row        1
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'gminfirst
        ?type       'boolean
        ?page       "Tolerance"
        ?row        2
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'noopiter
        ?type       'boolean
        ?page       "Tolerance"
        ?row        2
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'dcmu
        ?type       'string
        ?value      "0.5"
        ?min        0.0
        ?max        0.5
        ?page       "Tolerance"
        ?row        3
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'itl1
        ?type       'string
        ?value      "400"
        ?min        10
        ?max        1000
        ?page       "Tolerance"
        ?row        3
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'itl2
        ?type       'string
        ?value      "100"
        ?min        4
        ?max        500
        ?page       "Tolerance"
        ?row        4
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'itl4
        ?type       'string
        ?value      "10"
        ?min        4
        ?max        100
        ?page       "Tolerance"
        ?row        4
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'itl2gmin
        ?type       'string
        ?value      "20"
        ?min        4
        ?max        500
        ?page       "Tolerance"
        ?row        5
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'itl2src
        ?type       'string
        ?value      "20"
        ?min        4
        ?max        500
        ?page       "Tolerance"
        ?row        5
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'gmax
        ?type       'string
        ?value      "1.0e6"
        ?min        1.0e-2
        ?max        1.0e12
        ?page       "Tolerance"
        ?row        6
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'forcegmin
        ?type       'boolean
        ?page       "Tolerance"
        ?row        6
        ?column     2
    )

    ; Devices

    asiAddSimOption( tool 
        ?name       'defad
        ?type       'string
        ?value      "0.0"
        ?min        0.0
        ?page       "Devices"
        ?row        1
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'defas
        ?type       'string
        ?value      "0.0"
        ?min        0.0
        ?page       "Devices"
        ?row        1
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'defl
        ?type       'string
        ?value      "0.0"
        ?min        0.0
        ?page       "Devices"
        ?row        2
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'defw
        ?type       'string
        ?value      "0.0"
        ?min        0.0
        ?page       "Devices"
        ?row        2
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'bypass
        ?type       'cyclic
        ?choices    list("0" "1")
        ?value      "1"
        ?page       "Devices"
        ?row        3
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'useadjoint
        ?type       'boolean
        ?page       "Devices"
        ?row        3
        ?column     2
    )

    asiAddSimOption( tool 
        ?name       'oldlimit
        ?type       'boolean
        ?page       "Devices"
        ?row        4
        ?column     1
    )

    asiAddSimOption( tool 
        ?name       'trytocompact
        ?type       'boolean
        ?page       "Devices"
        ?row        4
        ?column     2
    )

    ; From the WRspice Temperature page, the temp option will be set
    ; from a separate ADE panel.
    asiAddSimOption( tool 
        ?name       'tnom
        ?type       'string
        ?value      "25.0"
        ?min        -273.15
        ?max        1000.0
        ?page       "Devices"
        ?row        5
        ?column     1
        ?genericName 'nominalTemp
    )
)

; This generates the .options string while netlisting.  We only include
; options set to a non-default value.
; TODO: set up range checking.  Mose options have min/max settings.
; What about .PARAM expansions?

defmethod( asiFormatSimulatorOptions (( session WRspice_session ) fp )
    let(( value (firstOption t))
        foreach( option asiGetSimOptionList( session )
            when( (value = asiGetFormattedVal( option ))
                (cond
                    ((type(value) == 'string && option->defValue != value)
                        when( firstOption
                            artFprintf(fp ".OPTIONS ")
                            firstOption=nil
                        )
                        artFprintf(fp "%s=%s " asiGetName(option) value)
                    )
                    ((value == t)
                        when( firstOption
                            artFprintf(fp ".OPTIONS ")
                            firstOption=nil
                        )
                        artFprintf(fp "%s " asiGetName(option))
                    )
                )
            )
        )
        t
    )
)

