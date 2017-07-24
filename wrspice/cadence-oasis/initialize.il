
; Whiteley Research Inc.  WRspice interface

defmethod( asiInitialize (( tool WRspice))
    asiInitAnalysis(tool)
    asiInitDataAccessFunction(tool)
    asiInitFormatterClass(tool)
    asiInitSimOption(tool)
    asiInitEnvOption(tool)
    t
)

