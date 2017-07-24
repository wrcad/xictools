; Support for WRspice
; Whiteley Research Inc. (http://wrcad.com)
; $Id:$

; Add parameters here that aren't already in the master params.il
; under $CDSHOME/tools/dfII/src/artist/analogLib.

; Noise factor for WRspice resistor.
procedure( almDefineParam_noise( _cellName )
    '( nil
       name          "noise"
       prompt        "Noise factor"
       type          "string"
       defValue      ""
       storeDefault  "no"
       parseAsCEL    "yes"
       parseAsNumber "yes"
    )
)

; Initial voltage of JJ device.
procedure( almDefineParam_vj( _cellName )
    '( nil
       name          "vj"
       prompt        "Initial voltage"
       type          "string"
       defValue      ""
       storeDefault  "no"
       parseAsCEL    "yes"
       parseAsNumber "yes"
    )
)
