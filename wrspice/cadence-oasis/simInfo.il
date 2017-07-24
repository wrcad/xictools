; Support for WRspice
; Whiteley Research Inc. (http://wrcad.com)
; $Id:$

;;================
;; V Sources
;;================

procedure( almCreateSimInfo_vdc_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVdc
    instParameters      (dc acm acp)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

procedure( almCreateSimInfo_iprobe_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVdc
    instParameters      (vdummy)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil dc vdummy)
    noPortDelimiter     t
    current             port
  )
)

procedure( almCreateSimInfo_vexp_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVexp
    instParameters      (dc acm acp v1 v2 td1 td2 tau1 tau2)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

procedure( almCreateSimInfo_vpulse_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVpulse
    instParameters      (dc acm acp v1 v2 td tr tf pw per)
;    instParameters      (dc acm acp v1 v2 td tr tf pw per dvals td1 td2 td3
;                        td4 td5 td6 td7 td8 td9 td10 td11 td12 td13 td14 td15
;                        td16 td17 td18 td19 td20)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

procedure( almCreateSimInfo_vpwl_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVpwl
    instParameters      (dc acm acp tvpairs t1 v1 t2 v2 t3 v3
                        t4 v4 t5 v5 t6 v6 t7 v7 t8 v8 t9 v9 t10 v10
                        t11 v11 t12 v12 t13 v13 t14 v14 t15 v15 t16 v16
                        t17 v17 t18 v18 t19 v19 t20 v20 t21 v21 t22 v22
                        t23 v23 t24 v24 t25 v25 t26 v26 t27 v27 t28 v28
                        t29 v29 t30 v30 t31 v31 t32 v32 t33 v33 t34 v34
                        t35 v35 t36 v36 t37 v37 t38 v38 t39 v39 t40 v40
                        t41 v41 t42 v42 t43 v43 t44 v44 t45 v45 t46 v46
                        t47 v47 t48 v48 t49 v49 t50 v50)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil td delay)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

procedure( almCreateSimInfo_vsffm_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVsffm
    instParameters      (dc acm acp vo va freq mdi fs)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

procedure( almCreateSimInfo_vsin_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVsin
    instParameters      (dc acm acp vo va freq td theta phi)
    componentName       vsrc
    namePrefix          "V"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
    current             port
  )
)

;;================
;; I Sources
;;================

procedure( almCreateSimInfo_idc_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIdc
    instParameters      (dc acm acp)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

procedure( almCreateSimInfo_iexp_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIexp
    instParameters      (dc acm acp i1 i2 td1 td2 tau1 tau2)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

procedure( almCreateSimInfo_ipulse_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIpulse
    instParameters      (dc acm acp i1 i2 td tr tf pw per)
;    instParameters      (dc acm acp i1 i2 td tr tf pw per dvals td1 td2 td3
;                        td4 td5 td6 td7 td8 td9 td10 td11 td12 td13 td14 td15
;                        td16 td17 td18 td19 td20)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

procedure( almCreateSimInfo_ipwl_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIpwl
    instParameters      (dc rpt td tvpairs acm acp t1 i1 t2 i2 t3 i3
                        t4 i4 t5 i5 t6 i6 t7 i7 t8 i8 t9 i9 t10 i10
                        t11 i11 t12 i12 t13 i13 t14 i14 t15 i15 t16 i16
                        t17 i17 t18 i18 t19 i19 t20 i20 t21 i21 t22 i22
                        t23 i23 t24 i24 t25 i25 t26 i26 t27 i27 t28 i28
                        t29 i29 t30 i30 t31 i31 t32 i32 t33 i33 t34 i34
                        t35 i35 t36 i36 t37 i37 t38 i38 t39 i39 t40 i40
                        t41 i41 t42 i42 t43 i43 t44 i44 t45 i45 t46 i46
                        t47 i47 t48 i48 t49 i49 t50 i50)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil td delay)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

procedure( almCreateSimInfo_isffm_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIsffm
    instParameters      (dc acm acp io ia freq mdi fs)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

procedure( almCreateSimInfo_isin_WRspice()
  '(nil
    netlistProcedure    WRspicePrintIsin
    instParameters      (dc acm acp io ia freq td theta phi)
    componentName       isrc
    namePrefix          "I"
    source              t
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              nil
  )
)

;;================
;; Dep. Sources
;;================

procedure( almCreateSimInfo_cccs_WRspice()
  '(nil
    netlistProcedure    WRspicePrintCCCS
    instParameters      (hfgain vref)
    componentName       cccs
    namePrefix          "F"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              nil
  )
)

procedure( almCreateSimInfo_ccvs_WRspice()
  '(nil
    netlistProcedure    WRspicePrintCCVS
    instParameters      (hhgain vref)
    componentName       ccvs
    namePrefix          "H"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              nil
  )
)

procedure( almCreateSimInfo_vccs_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVCCS
    refTermOrder        (NC\+ NC\-)
    instParameters      (hggain)
    componentName       vccs
    namePrefix          "G"
    termOrder           (PLUS MINUS NC\+ NC\-)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))" NC\+ "(FUNCTION zero(root(\"PLUS\")))" NC\- "(FUNCTION zero(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              nil
  )
)

procedure( almCreateSimInfo_vcvs_WRspice()
  '(nil
    netlistProcedure    WRspicePrintVCVS
    refTermOrder        (NC\+ NC\-)
    instParameters      (hegain)
    componentName       vcvs
    namePrefix          "E"
    termOrder           (PLUS MINUS NC\+ NC\-)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))" NC\+ "(FUNCTION zero(root(\"PLUS\")))" NC\- "(FUNCTION zero(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              nil
  )
)

;;================
;; Capacitor
;;================

procedure( almCreateSimInfo_cap_WRspice()
  '(nil
    netlistProcedure    WRspicePrintCap
    instParameters      (c ic w l temp tc1 tc2)
    otherParameters     (model)
    componentName       cap
    namePrefix          "C"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              t
    current             port
  )
)

;;================
;; Inductor
;;================

procedure( almCreateSimInfo_ind_WRspice()
  '(nil
    netlistProcedure    WRspicePrintInd
    instParameters      (l ic)
    componentName       ind
    namePrefix          "L"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_mind_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMind
    instParameters      (k)
    otherParameters     (ind1 ind2)
    componentName       mind
    namePrefix          "K"
    termOrder           nil
    propMapping         nil
    dcSens              nil
    acSens              nil
  )
)

;;================
;; Resistor
;;================

procedure( almCreateSimInfo_res_WRspice()
  '(nil
    netlistProcedure    WRspicePrintRes
    instParameters      (r temp w l tc1 tc2 noise)
    otherParameters     (model)
    componentName       res
    namePrefix          "R"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              t
    acSens              t
    current             port
  )
)

;;================
;; Josephson Jnc
;;================

procedure( almCreateSimInfo_jj_WRspice()
  '(nil
    netlistProcedure    WRspicePrintJJ
    instParameters      (area off phi vj)
    otherParameters     (model)
    componentName       jj
    namePrefix          "B"
    termOrder           (PLUS MINUS PH)
    termMapping         (nil PLUS "" MINUS "" PH "")
    propMapping         (nil)
    noPortDelimiter     t
    dcSens              nil
    acSens              nil
    current             port
  )
)

;;================
;; TLine
;;================

procedure( almCreateSimInfo_tline_WRspice()
  '(nil
    netlistProcedure    WRspicePrintTline
    otherParameters     (model)
    instParameters      (len l c r g zo td freq nl v1 i1 v2 i2)
    componentName       tline
    namePrefix          "T"
    termOrder           (IN\+ IN\- OUT\+ OUT\-)
    termMapping         (nil IN\+ "" IN\- "" OUT\+ "" OUT\- "")
    propMapping         (nil f freq)
    dcSens              nil
    acSens              nil
  )
)

;;================
;; Diode
;;================

procedure( almCreateSimInfo_diode_WRspice()
  '(nil
    netlistProcedure    WRspicePrintDiode
    instParameters      (off temp dtemp ic area pj m)
    otherParameters     (model)
    componentName       diode
    namePrefix          "D"
    termOrder           (PLUS MINUS)
    termMapping         (nil PLUS "" MINUS "(FUNCTION minus(root(\"PLUS\")))")
    propMapping         (nil)
    noPortDelimiter     t
  )
)

;;================
;; BJT
;;================

procedure( almCreateSimInfo_npn_WRspice()
  '(nil
    netlistProcedure    WRspicePrintBjt
    instParameters      (off temp dtemp area Vbe Vce)
    otherParameters     (model)
    componentName       npn
    namePrefix          "Q"
    termOrder           (C B E S)
    termMapping         (nil C \,C B \,B E \,E S \,S)
    propMapping         (nil icvbe Vbe icvce Vce)
    dataAccessMap       (IDC((B "ib") (C "ic")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pnp_WRspice()
  '(nil
    netlistProcedure    WRspicePrintBjt
    instParameters      (off temp dtemp area Vbe Vce)
    otherParameters     (model)
    componentName       npn
    namePrefix          "Q"
    termOrder           (C B E S)
    termMapping         (nil C \,C B \,B E \,E S \,S)
    propMapping         (nil)
    dataAccessMap       (IDC((B "ib") (C "ic")))
    dcSens              t
    acSens              t
    current             port
  )
)

;;================
;; MOS
;;================

procedure( almCreateSimInfo_nbsim_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_nbsim4_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_nmos_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_nmos4_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pbsim_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pbsim4_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pmos_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    termMapping         (nil D \,D G \,G S \,S B \,B)
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    dataAccessMap       (IDC((D "id")))
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pmos4_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMos
    instParameters      (l w ad as pd ps nrd nrs rdc rsc off Vds Vgs Vbs dtemp geo m)
    otherParameters     (model)
    componentName       mos
    namePrefix          "M"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil vds Vds vgs Vgs vbs Vbs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

;;================
;; JFET
;;================

procedure( almCreateSimInfo_njfet_WRspice()
  '(nil
    netlistProcedure    WRspicePrintJfet
    instParameters      (Vds Vgs area off temp)
    otherParameters     (model)
    componentName       jfet
    namePrefix          "J"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil icvds Vds icvgs Vgs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_pjfet_WRspice()
  '(nil
    netlistProcedure    WRspicePrintJfet
    instParameters      (Vds Vgs area off temp)
    otherParameters     (model)
    componentName       jfet
    namePrefix          "J"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil icvds Vds icvgs Vgs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

;;================
;; MESFET
;;================

procedure( almCreateSimInfo_nmes_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMes
    instParameters      (Vds Vgs area off temp)
    otherParameters     (model)
    componentName       mesfet
    namePrefix          "Z"
    termOrder           (D G S)
    termMapping         (nil D \,D G \,G S \,S)
    propMapping         (nil icvds Vds icvgs Vgs)
    dataAccessMap       (IDC((D "id")))
    dcSens              t
    acSens              t
    current             port
  )
)

procedure( almCreateSimInfo_nmes4_WRspice()
  '(nil
    netlistProcedure    WRspicePrintMes
    instParameters      (Vds Vgs area off temp)
    otherParameters     (model)
    componentName       mesfet
    namePrefix          "Z"
    termOrder           (D G S B)
    termMapping         (nil D \,D G \,G S \,S B \,B)
    propMapping         (nil icvds Vds icvgs Vgs)
    dataAccessMap       (IDC((D "id")))
  )
)

