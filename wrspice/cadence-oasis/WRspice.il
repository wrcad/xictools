; Support for WRspice
; Whiteley Research Inc. (http://wrcad.com)
; $Id:$

;;================
;; V Sources
;;================

(defun WRspicePrintVdc (inst)
  (_WRspicePrintSrcCommon inst)
  )

(defun WRspicePrintVexp (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " EXP(" )
    (_WRspicePrintRequiredInstParamList inst '(v1 v2 td1))
    (_WRspicePrintOrderedParameters inst '(td2 tau1 tau2) )
    (nlPrintString netlister ")" )
    ))

(defun WRspicePrintVpulse (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " PULSE(" )
    (_WRspicePrintRequiredInstParamList inst '(v1 v2 td))
    (_WRspicePrintOrderedParameters inst '(tr tf pw per) )
    (nlPrintString netlister ")" )
    ))

;  (let (dvals (netlister (nlGetNetlister inst)))
;    (_WRspicePrintSrcCommon inst)
;    
;    (nlPrintString netlister " PULSE(" )
;    (_WRspicePrintRequiredInstParamList inst '(v1 v2 td))
;    (_WRspicePrintOrderedParameters inst '(tr tf pw per)
;    dvals = (nlGetParamStringValue inst 'dvals)
;    dvals = (and dvals (round (evalstring dvals)))
;    (when (plusp dvals)
;      (for
;       cntr 1 dvals
;       (_WRspicePrintRequiredInstParam inst (concat 'td cntr ))
;       )
;      (nlPrintString netlister " )")
;      ))))

(defun WRspicePrintVpwl (inst)
  (let (pairs (netlister (nlGetNetlister inst)) )
    (_WRspicePrintSrcCommon inst)
    pairs = (nlGetParamStringValue inst 'tvpairs)
    pairs = (and pairs (round (evalstring pairs)))
    (when (plusp pairs)
      (nlPrintString netlister " PWL(" )
      (for
       cntr 1 pairs
       (_WRspicePrintRequiredInstParam inst (concat 't cntr ))
       (_WRspicePrintRequiredInstParam inst (concat 'v cntr))
       )
      (nlPrintString netlister " )")
      )))

(defun WRspicePrintVsffm (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " SFFM(" )
    (_WRspicePrintRequiredInstParamList inst '(vo va))
    (_WRspicePrintOrderedParameters inst '(freq mdi fs) )
    (nlPrintString netlister ")" )
    ))

(defun WRspicePrintVsin (inst) 
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " SIN(" )
    (_WRspicePrintRequiredInstParamList inst '(vo va))
    (_WRspicePrintOrderedParameters inst '(freq td theta phi) )
    (nlPrintString netlister ")" )
    ))

;;================
;; I Sources
;;================

(defun WRspicePrintIdc (inst)
  (_WRspicePrintSrcCommon inst)
  )

(defun WRspicePrintIexp (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " EXP(" )
    (_WRspicePrintRequiredInstParamList inst '(i1 i2 td1))
    (_WRspicePrintOrderedParameters inst '(td2 tau1 tau2) )
    (nlPrintString netlister ")" )
    ))

(defun WRspicePrintIpulse (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " PULSE(" )
    (_WRspicePrintRequiredInstParamList inst '(i1 i2 td))
    (_WRspicePrintOrderedParameters inst '(tr tf pw per) )
    (nlPrintString netlister ")" )
    ))

;  (let (dvals (netlister (nlGetNetlister inst)))
;    (_WRspicePrintSrcCommon inst)
;    
;    (nlPrintString netlister " PULSE(" )
;    (_WRspicePrintRequiredInstParamList inst '(i1 i2 td))
;    (_WRspicePrintOrderedParameters inst '(tr tf pw per)
;    dvals = (nlGetParamStringValue inst 'dvals)
;    dvals = (and dvals (round (evalstring dvals)))
;    (when (plusp dvals)
;      (for
;       cntr 1 dvals
;       (_WRspicePrintRequiredInstParam inst (concat 'td cntr ))
;       )
;      (nlPrintString netlister " )")
;      ))))

(defun WRspicePrintIpwl (inst)
  (let (pairs (netlister (nlGetNetlister inst)) )
    (_WRspicePrintSrcCommon inst)
    pairs = (nlGetParamStringValue inst 'tvpairs)
    pairs = (and pairs (round (evalstring pairs)))
    (when (plusp pairs)
      (nlPrintString netlister " PWL(" )
      (for
       cntr 1 pairs
       (_WRspicePrintRequiredInstParam inst (concat 't cntr ))
       (_WRspicePrintRequiredInstParam inst (concat 'i cntr))
       )
      (nlPrintString netlister " )")
      )))

(defun WRspicePrintIsffm (inst)
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " SFFM(" )
    (_WRspicePrintRequiredInstParamList inst '(io ia))
    (_WRspicePrintOrderedParameters inst '(freq mdi fs) )
    (nlPrintString netlister ")" )
    ))

(defun WRspicePrintIsin (inst) 
  (let ( (netlister (nlGetNetlister inst)))
    (_WRspicePrintSrcCommon inst)
    
    (nlPrintString netlister " SIN(" )
    (_WRspicePrintRequiredInstParamList inst '(io ia))
    (_WRspicePrintOrderedParameters inst '(freq td theta phi) )
    (nlPrintString netlister ")" )
    ))

;;================
;; Dep. Sources
;;================

(defun WRspicePrintCCCS (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintReferenceParam inst 'vref t)
  (_WRspicePrintRequiredInstParam inst 'hfgain)
  )

(defun WRspicePrintCCVS (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintReferenceParam inst 'vref t)
  (_WRspicePrintRequiredInstParam inst 'hhgain)
  )

(defun WRspicePrintVCCS (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintRequiredInstParam inst 'hhgain)
  )

(defun WRspicePrintVCVS (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintRequiredInstParam inst 'hegain)
  )

;;================
;; Capacitor
;;================

(defun WRspicePrintCap (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintRequiredInstParam inst 'c)
  (_WRspicePrintInstParamList inst '(ic w l temp tc1 tc2) ?paramPrint t)
  )
;  (let ( (formatter (nlGetFormatter inst)) )
;    (_WRspicePrintInstCommon inst)
;    (nlPrintModelName formatter inst)
;    (_WRspicePrintInstParamList inst '(c ic w l temp tc1 tc2) ?paramPrint t)
;    ))

;;================
;; Inductor
;;================

(defun WRspicePrintInd (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintRequiredInstParam inst 'l)
  (_WRspicePrintInstParam inst 'ic ?paramPrint t)
  )

(defun WRspicePrintMind (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintReferenceParamList inst '(ind1 ind2) t)
  (_WRspicePrintRequiredInstParam inst 'k) 
  )
  
;;================
;; Resistor
;;================

(defun WRspicePrintRes (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintRequiredInstParam inst 'r)
  (_WRspicePrintInstParamList inst '(temp w l tc1 tc2 noise) ?paramPrint t)
  )
;  (let ( (formatter (nlGetFormatter inst)) )
;    (_WRspicePrintInstCommon inst)
;    (nlPrintModelName formatter inst)
;    (_WRspicePrintInstParamList inst '(r temp w l tc1 tc2 noise) ?paramPrint t)
;    ))

;;================
;; Josephson Jnc
;;================

(defun WRspicePrintJJ (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintInstParam inst 'area ?paramPrint t ?defaultVal "1.0")
    (_WRspicePrintOff inst)
    (_WRspicePrintIC inst 'vj 'phi)
    ))

;;================
;; TLine
;;================

(defun WRspicePrintTline (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintInstParamList inst '(len l c r g zo td freq nl v1 i1 v2 i2) ?paramPrint t)
    ))

;;================
;; Diode
;;================

(defun WRspicePrintDiode (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintInstParam inst 'area ?defaultVal "1.0")
    (_WRspicePrintOff inst)
    (_WRspicePrintInstParamList inst '(temp dtemp ic pj m) ?paramPrint t)
    ))

;;================
;; BJT
;;================

(defun WRspicePrintBjt (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintInstParam inst 'area ?defaultVal "1.0")
    (_WRspicePrintOff inst)
    (_WRspicePrintIC inst 'icvbe 'icvce)
    (_WRspicePrintInstParam inst 'temp ?paramPrint t)
    (_WRspicePrintInstParam inst 'dtemp ?paramPrint t)
    ))

;;================
;; MOS
;;================

(defun WRspicePrintMos (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintOff inst)
    (_WRspicePrintIC inst 'vds 'vgs 'vbs)
    (_WRspicePrintInstParamList inst '(l w ad as pd ps nrd nrs
     rdc rsc dtmp geo m) ?paramPrint t)
    ))

;;================
;; JFET
;;================

(defun WRspicePrintJfet (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintOff inst)
    (_WRspicePrintIC inst 'icvds 'icvgs)
    (_WRspicePrintInstParamList inst '(area temp) ?paramPrint t)
    ))

;;================
;; MESFET
;;================

(defun WRspicePrintMes (inst)
  (let ( (formatter (nlGetFormatter inst)) )
    (_WRspicePrintInstCommon inst)
    (nlPrintModelName formatter inst)
    (_WRspicePrintOff inst)
    (_WRspicePrintIC inst 'icvds 'icvgs)
    (_WRspicePrintInstParamList inst '(area temp) ?paramPrint t)
    ))


;;;;;;;;;;;;;;;;;;;;;;
;; Utility functions
;;;;;;;;;;;;;;;;;;;;;;
    
(defun _WRspicePrintSrcCommon (inst)
  (_WRspicePrintInstCommon inst)
  (_WRspicePrintDC inst)
  (_WRspicePrintAC inst)
  )  
    
(defun _WRspicePrintInstCommon (inst)
  (let ( (formatter (nlGetFormatter inst)))
    (nlPrintInstComments formatter inst)
    (nlPrintIndentString (nlGetNetlister formatter))
    (nlPrintInstName formatter inst)
    (nlPrintInstSignals formatter inst)
    ))
    
(defun _WRspicePrintInstParam (inst param 
				   @key (paramPrint nil) (defaultVal nil))
  (let ( (val (nlGetParamStringValue inst param))
	 (nl (nlGetNetlister inst)))
    (when (or val defaultVal)
      (nlPrintString nl " ")
      (when paramPrint
	(nlPrintString nl (sprintf nil "%s=" (upperCase param))))
      (nlPrintString nl (or val (get_string defaultVal)))
      )
    ))
    
(defun _WRspicePrintInstParamList (inst paramList 
				       @key (paramPrint nil) (defaultVal nil))
  (foreach 
   param paramList
   (_WRspicePrintInstParam inst param ?paramPrint paramPrint 
			  ?defaultVal defaultVal)
   ))

(defun _WRspicePrintRequiredInstParam (inst param)
  (let ((val (nlGetParamStringValue inst param))
	(nl (nlGetNetlister inst)))
    (cond
     ((null val)
      (nlError nl "Missing parameter '%s' in %s\n" param (nlGetSimName inst)))
     (t (nlPrintString nl " " val))
     )))

(defun _WRspicePrintRequiredInstParamList (inst paramList)
  (foreach 
   param paramList
   (_WRspicePrintRequiredInstParam inst param)
   ))
    
(defun _WRspicePrintOrderedParameters (inst paramList)
  (let (nilValList val (nl (nlGetNetlister inst)))
    (while paramList
      val = (nlGetParamStringValue inst (car paramList))
      (cond
       ( (and val nilValList)
	 (nlError nl "Missing parameters %L in %s\n" 
		  nilValList (nlGetSimName) inst))
       ( val
	 (nlPrintString nl " " val))
       ( t
	 nilValList = (cons (car paramList) nilValList))
       )
      paramList = (cdr paramList)
      )
    ))
    
(defun _WRspicePrintAC (inst)   
  (let ( (nl (nlGetNetlister inst))
	 (acmag (nlGetParamStringValue inst 'acm)) acphase)
  (when acmag
    (nlPrintString nl " AC" )
    (nlPrintString nl " " acmag)
    (when acphase = (nlGetParamStringValue inst 'acp)
	  (nlPrintString nl " " acphase))
    )
  ))
    
(defun _WRspicePrintDC (inst)
  (let ( (dcVal (nlGetParamStringValue inst 'vdc))
	 (nl (nlGetNetlister inst)))
    (when dcVal (nlPrintString nl " " dcVal))
    ))
    
(defun _WRspicePrintOff (inst)
  (let ( (off (nlGetParamStringValue inst 'off) )
	 (nl (nlGetNetlister inst) ))
    (when (equal off "TRUE") (nlPrintString nl " OFF" ))
    ))

(defun _WRspicePrintIC (inst @rest args)
  (let ( (nl (nlGetNetlister inst)))
    (when (exists arg args (nlGetParamStringValue inst arg))
      (nlPrintString nl " IC=")
      (_WRspicePrintInstParamList inst args ?defaultVal "0.0")
      )))

(defun _WRspicePrintReferenceParam (inst param @optional required prefix)
  (let ((nl (nlGetNetlister inst))(val (nlGetParamStringValue inst param)) map)
    (cond
     ((and required (null val))
      (nlObjError nl inst "Missing value for parameter '%s'" param))
     ((and val (null map = (nlGetInstSimName nl val)))
      (nlObjError
       nl inst "Cannot resolve value %L of parameter '%s' to a simulator name"
       val param))
     ((and map prefix) (nlPrintString nl prefix map))
     (map (nlPrintString nl " " val))
     )))

(defun _WRspicePrintReferenceParamList (inst paramList @optional required)
  (foreach 
   param paramList
   (_WRspicePrintReferenceParam inst param required)
   ))
