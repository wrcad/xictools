; Support for WRspice
; Whiteley Research Inc. (http://wrcad.com)
; $Id:$

defmethod( asiInitEnvOption (( tool WRspice ))
    asiAddEnvOption( tool
        ?name       'switchViewList
        ?type       'list
        ?value      '("WRspice" "cmos_sch" "cmos.sch" "schematic")
    )
    asiAddEnvOption( tool
        ?name       'stopViewList
        ?type       'list
        ?value      '("WRspice")
    )
)
