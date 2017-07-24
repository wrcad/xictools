;
;                             Copyright (C) 1993
;                     Cadence Design Systems, Analog Division
;                            All Rights Reserved.
;
;  $Source: /cvsrep/group/artist/src/spice3/analysis.il,v $
;  $Author: p4admin $
;  $Revision: #1 $
;  $Date: 2014/12/01 $
;  $State: Exp $
;

; Adapted for WRspice, Whiteley Research Inc.
; $Id:$
 
defmethod( asiInitAnalysis (( tool WRspice ))

    asiSetAnalysisFormWidth( tool 600 )

    ;;; *******************************************************
    ;;; DC SWEEP 
    ;;; *******************************************************
	
    asiAddAnalysis( tool
	    ?name 'dc
	    ?prompt "DC Sweep"
	    ?formType 'twoD
	    ?fieldList list(
		asiCreateAnalysisField(
		    ?name	'vstart
		    ?type	'numericString	
		    ?prompt	"Start"
                    ?row        1
                    ?column     1
		)
		asiCreateAnalysisField( 
		    ?name	'vstop
		    ?type	'numericString	
		    ?prompt	"Stop"
                    ?row        1
                    ?column     2
		)
		asiCreateAnalysisField( 
		    ?name	'vincr
		    ?type	'numericString	
		    ?prompt	"By"
                    ?row        1
                    ?column     3
		)
		asiCreateAnalysisField(
		    ?name	'srcnam
		    ?prompt	"Source Name"
		    ?type	'instance	
                    ?row        2
                    ?column     1
                    ?width	2
		)

		;;; Source select push button...
		asiCreateAnalysisField(
		    ?name 'selectSource
		    ?prompt "Select Source"
                    ?type 'button
		    ?callback	"asiSelectAnalysisSource( 'dc 'srcnam \"Select a dc source ...\" )"
                    ?row        2
                    ?column     2
		)

	    )
    )

    ;;; *******************************************************
    ;;; AC/FREQUENCY DOMAIN ANALYSIS
    ;;; *******************************************************
	
    asiAddAnalysis( tool
	    ?name 'ac
	    ?prompt "AC"
	    ?formType 'twoD
	    ?fieldList list(
		asiCreateAnalysisField(
		    ?name	'fstart
		    ?type	'numericString	
		    ?prompt	"From (Hz)"
                    ?row        1
                    ?column     1
		)
		asiCreateAnalysisField( 
		    ?name	'fstop
		    ?type	'numericString	
		    ?prompt	"To (Hz)"
                    ?row        1
                    ?column     2
		)
		asiCreateAnalysisField( 
		    ?name	'incrType
                    ?prompt	"Sweep Type"
		    ?type	'radio
		    ?choices	'( "LIN" "DEC" )
		    ?oceanChoices '("L" "D")
		    ?value 	"DEC"
                    ?row        2
                    ?column     1
		)
		asiCreateAnalysisField( 
		    ?name	'log
		    ?type	'numericString	
		    ?prompt	"Points per Decade"
                    ?row        2
                    ?column     2
                    ?display	'equal( asiGetAnalysisFormFieldVal( form 'ac 'incrType ) "DEC" )
		)
		asiCreateAnalysisField( 
		    ?name	'lin
		    ?type	'numericString	
		    ?prompt	"Step Size (Hz)"
                    ?row        2
                    ?column     2
                    ?display	'equal( asiGetAnalysisFormFieldVal( form 'ac 'incrType ) "LIN" )
		)
	    )
    )

    ;;; *******************************************************
    ;;; TRANSIENT/TIME DOMAIN ANALYSIS
    ;;; *******************************************************

    asiAddAnalysis( tool
	    ?name 'tran
	    ?prompt "Transient"
	    ?formType 'twoD
	    ?fieldList list(
		asiCreateAnalysisField(
		    ?name	'tstart
		    ?type	'numericString	
		    ?prompt	"Start Time"
                    ?row        1
                    ?column     1
		)
		asiCreateAnalysisField(
		    ?name	'tstop
		    ?type	'numericString	
		    ?prompt	"Stop Time"
                    ?row        1
                    ?column     2
		)
		asiCreateAnalysisField(
		    ?name	'tstep
		    ?type	'numericString	
		    ?prompt	"Step"
                    ?row        2
                    ?column     1
		)
		asiCreateAnalysisField(
		    ?name	'tmax
		    ?type	'numericString	
		    ?prompt	"Max Step Size"
                    ?row        2
                    ?column     2
		)
		asiCreateAnalysisField(
		    ?name	'uic
		    ?type	'boolean	
		    ?prompt	"Use Initial Conditions"
                    ?row        3
                    ?column     1
		)
	    )
    )
    t 
)

/*************************************************************************
*                                                                        *
*                          FORMAT  METHODS                               *
*                                                                        *
*************************************************************************/

/***********************Analysis List**********************************
* EXTDOC: asiFormatAnalysisList
*
* PROTOTYPE: ( o_session fp_controlStatementsFile ) -> t
*
* INTERNAL: Write out the native sweep/loop control statements 
*           (altergroup statements) for Corner's Analysis.
*           This can be modifed to write out Monte Carlo analysis 
*           statements also.
*
* ORIGINAL: Vikas Arora
*
***********************************************************************/

defmethod( asiFormatAnalysisList (( session WRspice_session ) fp )
 (let (( cornerList reverse(asiGetCornerList( session ))) (alterMode nil ) paramList warnMsg paramName paramVal)

  ;; write out the analyses statements for the defined Analyses
  (callNextMethod)

  ;; Write out sweep control (altergroup) statements for Corner's Analysis
  when( and(cornerList asiGetEnvOptionVal(session 'useAltergroup))

     ;; Integrators should Change the following sweep control statements 
     ;; (the code in this foreach block) according to their simulators.
     foreach( corner cornerList
       ;; Internal Function used for testing purpose only
       _WRspiceSwitchLang1(session fp)

       ;; Print the ALTER statement
       artFprintf( fp "%s %s\n" get_pname( asiGetCornerName( corner )) _WRspiceStartAlterKeyWord(session))

       ;; print design variable list for that Corner
       when( paramList = asiGetCornerDesignVarList( corner )
         artFprintf( fp "%s" _WRspiceParamDefineKeyWord(session)) 
         foreach( param paramList
           paramName = car(param )
           paramVal = cadr( param )
           cond(
               ( artBlankString( paramName )
                   artWarn("a blank parameter name is not included in the input file."))
               ( artBlankString( paramVal )
                   warnMsg = strcat( paramName " does not have a value, and therefore is not included in the input file.")
                   artWarn( warnMsg ))
               ( t artFprintf( fp " %s=%s" paramName paramVal ))
          )
         ) ;; foreach
         artFprintf( fp "\n" )
       ) ;; when

       ;; print model include statements for that Corner
       foreach( selection asiGetCornerModelSelectionList( corner )
         cond(
             ( onep( length( selection ))
                 artFprintf( fp "%s %L\n" _WRspiceIncludeModelKeyWord(session) car( selection )))

             ( t
                 artFprintf( fp "%s %L section=%s\n" _WRspiceIncludeModelKeyWord(session) car( selection ) cadr( selection )))
         )
       ) ;; foreach

       artFprintf( fp "%s\n" _WRspiceEndAlterKeyWord(session))

       ;; Internal Function, used for testing purpose only
       _WRspiceSwitchLang2(session fp)

       ;; print Simulation Temperature for that Corner
       (artFprintf fp ".OPTIONS TEMP=%s\n" asiGetCornerTemperature(corner))

       ;; Print the simulation analysis again for that Corner. 
       ;; Depending on the .ALTER construct of your Simulator, the defined
       ;; analyses might have to be re-run again for the altered models/params
       ;; In case your simulator evaluates/runs the Analyses on it's own after
       ;; the model/params are altered, you need not call this again. 
       (callNextMethod)
     ) ;; foreach
   ) ;; when
  t
 )
)

/*****************************************************************************
* EXTDOC:	asiFormatAnalysis
* CATEGORY:	analog 
*
* PROTOTYPE:	( o_analog_tran_analysis fp/nil ) -> t/nil
*
* INTERNAL:	Generates the statements necessary for a typical analog
*		simulator's transient analysis.  
*
******************************************************************************/
defmethod( asiFormatAnalysis ( ( analysis WRspice_tran_analysis ) fp )
  let(( value )
    artFprintf( fp ".TRAN " )
    foreach( f list( 'tstep 'tstop 'tstart 'tmax ) 
      when( value = asiGetFormattedVal( asiGetAnalysisField( analysis f)) 
        artFprintf( fp "%s " value ) 
      )
    )
    when( value = asiGetFormattedVal( asiGetAnalysisField( analysis `uic)) 
      artFprintf( fp "%s " "uic" ) 
    )
    artFprintf( fp "\n" )
    t 
  )
)

defmethod( asiFormatAnalysis ( ( analysis WRspice_ac_analysis ) fp )
  let(( value )
    artFprintf( fp ".AC " )
    when( value = asiGetFormattedVal(asiGetAnalysisField(analysis 'incrType))
      artFprintf( fp "%s " value ) 
    )
    cond( 
      ( equal( value "DEC" )	
	artFprintf(fp "%s " asiGetFormattedVal(
	    asiGetAnalysisField( analysis 'log))))
      ( equal( value "LIN" )
	artFprintf(fp "%s " asiGetFormattedVal(
	    asiGetAnalysisField( analysis 'lin))))
      (t t)	
    )
    foreach( f list('fstart 'fstop )  
      when( value = asiGetFormattedVal( asiGetAnalysisField( analysis f ))
        artFprintf( fp "%s " value ) 
      )
    )
    artFprintf( fp "\n" )
    t
  )
)

defmethod( asiFormatAnalysis (( analysis WRspice_dc_analysis ) fp )
  let(( value )
    artFprintf( fp ".DC " )
    foreach( f list( 'srcnam 'vstart 'vstop 'vincr )	
      when( value = asiGetFormattedVal(asiGetAnalysisField(analysis f)) 
        artFprintf(fp "%s " value )
      )	
    )
    artFprintf( fp "\n" )
    t
  )
)

defmethod( asiCheck (( ana WRspice_tran_analysis ) form )
  asiCheckBlank( ana form 'tstep )
  asiCheckBlank( ana form 'tstop )
)

defmethod( asiCheck (( ana WRspice_ac_analysis ) form )
  asiCheckBlank( ana form 'fstart )
  asiCheckBlank( ana form 'fstop )
  if( equal( asiGetAnalysisFormFieldVal( form 'ac 'incrType ) "LIN" ) then
    asiCheckBlankNumericGreater( ana form 'lin 0 )
  else
    asiCheckBlankNumericGreater( ana form 'log 0 )
  )
 
  ;;; from and to
  asiCheckMultipleGreater( ana form 'fstop 'fstart )

)

defmethod( asiCheck (( ana WRspice_dc_analysis ) form )
  asiCheckBlank( ana form 'vstart )
  asiCheckBlank( ana form 'vstop )
  asiCheckBlank( ana form 'vincr )
)
