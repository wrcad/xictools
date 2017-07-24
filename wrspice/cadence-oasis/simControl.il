;
;                             Copyright (C) 1993
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/simControl.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$

(defmethod asiGetSimExecName ( (session WRspice_session) )
  "wrspice"
  )

(defmethod asiRunSimulation ( (session WRspice_session) )
  callNextMethod()
  )

(defmethod asiGetPredefinedCommandLineOption ( (session WRspice_session ))
 (let ( command )
   (sprintf command " -b -r psf@../psf" )
   command
 )
)

(defmethod asiGetCommandFooter ( (session WRspice_session) )
  ""
  )

(defmethod asiFormatModelLibSelectionList ((session WRspice_session) fp )
  (let ( modelLibList fileName section (cornerList asiGetCornerList(session ))
      absFileName printFile (modelPath asiGetEnvOptionVal(session 'includePath)))

    ;; In case of Corners, get the model path set by Corner tool
    if(cornerList then
      modelLibList = asiGetCornerModelSelectionList(car(cornerList))
    else
      modelLibList = (asiGetModelLibSelectionList session)
    )

    (foreach
     obj modelLibList
     fileName = (asiGetModelLibFile obj)
     section = (asiGetModelLibSection obj)

     ;; Internal Function, used for testing purpose only
     _WRspiceSwitchLang1(session fp)

     printFile=nil
     ;; If file exists, then print the model include statement for that file
     if(isFile(fileName) 
      then
       printFile=t
       artFprintf( fp "%s %L" _WRspiceIncludeModelKeyWord(session) fileName)
       (unless (blankstrp section) artFprintf( fp " section=%s" section ))
       (artFprintf fp "\n")
     )

     ;; Incase model file doesn't exist, it could be a relative file.
     ;; Try to form a complete path name for modelFile from the 'modelPath'
     unless( or( printFile blankstrp(modelPath)) 
       ;; model path (modelPath) is not null, 
       ;; So, the model file 'fileName' could be relative wrt, one of the model
       ;; paths in the 'modelPath' list. 
       ;; Try checking every model_path/model_file combination and print the
       ;; model include statement for the first combination that exist.
       (foreach
        dir parseString(modelPath)
        absFileName = strcat(simplifyFilename(dir) "/" fileName)
        if( and( null(printFile) isFile(absFileName)) 
          then 
            printFile=t
            artFprintf( fp "%s %L" _WRspiceIncludeModelKeyWord(session) absFileName)
            (unless (blankstrp section) artFprintf( fp " section=%s" section ))
            (artFprintf fp "\n")
        ) ;; if
       ) ;; foreach modelPath
     ) ;; unless

     ;; Internal Function, used for testing purpose only
     _WRspiceSwitchLang2(session fp)
    ) ;; foreach modelLibList
    t
  )
)
