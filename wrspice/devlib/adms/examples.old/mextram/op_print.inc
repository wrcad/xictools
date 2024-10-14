// print the operating point (outout) variables
begin 

`ifdef __VAMS_COMPACT_MODELING__

// The external voltage differences
$strobe("OP_vbe : ", OP_vbe) ;  // External base-emiter bias
$strobe("OP_vce : ", OP_vce) ;  // External collector-emiter bias
$strobe("OP_vbc : ", OP_vbc) ;  // External base-collector bias

`ifdef SUBSTRATE
$strobe("OP_vse : ", OP_vse) ;  // External substrate-emiter bias
$strobe("OP_vbs : ", OP_vbs) ;  // External base-substrate bias
$strobe("OP_vsc : ", OP_vsc) ;  // External substrate-collector bias
`endif

// The external currents and the current gain
$strobe("OP_ic : ", OP_ic);        // External DC collector current
$strobe("OP_ib : ", OP_ib);        // External DC base Current

// begin added in MXT 504.9:
$strobe("OP_ie : ", OP_ie);        // External DC emitter current
`ifdef SUBSTRATE
$strobe("OP_is : ", OP_is);        // External DC substrate current
`endif
// end added in MXT 504.9
$strobe("OP_betadc :", OP_betadc) ;   // External DC Current gain

// The internal voltage differences
$strobe("OP_vb2e1 : ", OP_vb2e1) ;  // Internal base-emiter bias
$strobe("OP_vb2c2 : ", OP_vb2c2) ;  // Internal base-emiter bias 
$strobe("OP_vb2c1 : ", OP_vb2c1) ;  // Internal base-collector bias including epilayer

$strobe("OP_vb1c1 : " ,OP_vb1c1) ;  // External base-collector bias without contact resistances

$strobe("OP_vc4c1 : ", OP_vc4c1) ;  // Bias over intrinsic buried layer
$strobe("OP_vc3c4 : ", OP_vc3c4) ;  // Bias over extrinsic buried layer

$strobe("OP_ve1e  : ", OP_ve1e) ;   // Bias over emiter resistance

// The branch currents
$strobe("OP_in    : ", OP_in)    ;         // Main current
$strobe("OP_ic1c2 : ", OP_ic1c2) ;         // Epilayer current
$strobe("OP_ib1b2 : ", OP_ib1b2) ;         // Pinched-base current
$strobe("OP_ib1   : ", OP_ib1)   ;         // Ideal forward base current
$strobe("OP_sib1  : ", OP_sib1)  ;         // Ideal side-wall base current
$strobe("OP_izteb  : ", OP_izteb)  ;         // Zener tunneling current in the emitter base junction
$strobe("OP_ib2   : ", OP_ib2)   ;         // Non-ideal forward base current
$strobe("OP_ib3   : ", OP_ib3)   ;         // Non-ideal reverse base current
$strobe("OP_iavl  : ", OP_iavl)  ;         // Avalanche current
$strobe("OP_iex   : ", OP_iex)   ;         // Extrinsic reverse base current
$strobe("OP_xiex  : ", OP_xiex)  ;         // Extrinsic reverse base current
`ifdef SUBSTRATE
$strobe("OP_isub  : ", OP_isub)  ;          // Substrate current
$strobe("OP_xisub : ", OP_xisub) ;         // Substrate current
$strobe("OP_isf   : ", OP_isf)   ;           // Substrate-collector current
`endif
$strobe("OP_ire   : ", OP_ire)   ;  // Current through emiter resistance
$strobe("OP_irbc  : ", OP_irbc)  ; // Current through constant base resistance

$strobe("OP_ircc  : ", OP_ircc)  ; // Current through collector contact resistance
$strobe("OP_ircblx: ", OP_ircblx) ; // Current through extrinsic buried layer resistance
$strobe("OP_ircbli: ", OP_ircbli) ; // Current through extrinsic buried layer resistance

// The branch charges
$strobe("OP_qe    : ", OP_qe      ) ; // Emitter charge or emitter neutral charge
$strobe("OP_qte   : ", OP_qte     ) ; // Base-emiter depletion charge
$strobe("OP_sqte  : ", OP_sqte    ) ; // Sidewall base-emiter depletion charge
$strobe("OP_qbe   : ", OP_qbe     ) ; // Base-emiter diffusion charge
$strobe("OP_qbc   : ", OP_qbc     ) ; // Base-collector diffusion charge
$strobe("OP_qtc   : ", OP_qtc     ) ; // Base-colector depletion charge
$strobe("OP_qepi  : ", OP_qepi    ) ; // Epilayer diffusion charge 
$strobe("OP_qb1b2 : ", OP_qb1b2   ) ; // AC current crowding charge
$strobe("OP_qtex  : ", OP_qtex    ) ; // Extrinsic base-collector depletion charge
$strobe("OP_xqtex : ", OP_xqtex   ) ; // Extrinsic base-collector depletion charge
$strobe("OP_qex   : ", OP_qex     ) ; // Extrinsic base-collector diffusion charge
$strobe("OP_xqex  : ", OP_xqex    ) ; // Extrinsic base-collector diffusion charge
`ifdef SUBSTRATE
$strobe("OP_qts   : ", OP_qts     ) ; // Collector substrate depletion charge
`endif



// Small signal equivalent circuit conductances and resistances

$strobe("OP_gx     : ",  OP_gx   ) ; // Forward transconductance

$strobe("OP_gy     : ",OP_gy   ) ; // Reverse transconductance
$strobe("OP_gz     : ",OP_gz   ) ; // Reverse transconductance
$strobe("OP_sgpi   : ",OP_sgpi   ) ; // Conductance sidewal b-e junction
$strobe("OP_gpix   : ",OP_gpix   ) ; // Conductance floor b-e junction

$strobe("OP_gpiy   : ",OP_gpiy   ) ; // Early effect on recombination base current
$strobe("OP_gpiz   : ",OP_gpiz   ) ; // Early effect on recombination base current

$strobe("OP_gmux   : ",OP_gmux   ) ; // Early effect on avalanche current limitting
$strobe("OP_gmuy   : ",OP_gmuy   ) ; // Conductance of avalanche current
$strobe("OP_gmuz   : ",OP_gmuz   ) ; // Conductance of avalanche current
$strobe("OP_gmuex  : ",OP_gmuex   ) ; // Conductance extrinsic b-c current
$strobe("OP_xgmuex : ",OP_xgmuex   ) ; // Conductance extrinsic b-c current

$strobe("OP_grcvy  : ",OP_grcvy   ) ; // Conductance of epilayer current
$strobe("OP_grcvz  : ",OP_grcvz   ) ; // Conductance of epilayer current

$strobe("OP_rbv    : ",OP_rbv   ) ; // Base resistance

$strobe("OP_grbvx  : ",OP_grbvx   ) ; // Early effect on base resistance
$strobe("OP_grbvy  : ",OP_grbvy   ) ; // Early effect on base resistance
$strobe("OP_grbvz  : ",OP_grbvz    ) ; // Early effect on base resistance
$strobe("OP_re     : ",OP_re   ) ; // Emiter resistance
$strobe("OP_rbc    : ",OP_rbc   ) ; // Constant base resistance
$strobe("OP_rcc    : ",OP_rcc   ) ; // Collector Contact resistance
$strobe("OP_rcblx  : ",OP_rcblx   ) ; // Extrinsic buried layer resistance
$strobe("OP_rcbli  : ",OP_rcbli   ) ; // Extrinsic buried layer resistance


`ifdef SUBSTRATE
$strobe("OP_gs     : ", OP_gs   ) ; // Conductance parasitic PNP transitor
$strobe("OP_xgs    : ", OP_xgs  ) ; // Conductance parasitic PNP transistor
$strobe("OP_gsf    : ", OP_gsf  ) ; // Conductance substrate failure current
`endif

// Small signal equivalent circuit capacitances
$strobe("OP_scbe   : ", OP_scbe    ) ; // Capacitance sidewall b-e junction
$strobe("OP_cbex   : ", OP_cbex    ) ; // Capacitance floor b-e junction
$strobe("OP_cbey   : ", OP_cbey   ) ; // Early effect on b-e diffusion junction
$strobe("OP_cbez   : ", OP_cbez   ) ; // Early effect on b-e diffusion junction
$strobe("OP_cbcx   : ", OP_cbcx   ) ; // Early effect on b-c diffusion junction
$strobe("OP_cbcy   : ",  OP_cbcy  ) ; // Capacitance floor b-c junction
$strobe("OP_cbcz   : ", OP_cbcz   ) ; // Capacitance floor b-c junction
$strobe("OP_cbcex  : ", OP_cbcex  ) ; // Capacitance extrinsic b-c junction 
$strobe("OP_xcbcex : ", OP_xcbcex ) ; // Capacitance extrinsic b-c junction
$strobe("OP_cb1b2  : ", OP_cb1b2    ) ; // Capacitance AC current crowding
$strobe("OP_cb1b2x : ", OP_cb1b2x   ) ; // Cross-capacitance AC current crowding
$strobe("OP_cb1b2y : ", OP_cb1b2y   )      ; // Cross-capacitance AC current crowding
$strobe("OP_cb1b2z : ", OP_cb1b2z   ) ; // Cross-capacitance AC current crowding
`ifdef SUBSTRATE
$strobe("OP_cts    : ", OP_cts   ) ; // Capacitance s-c junction
`endif
$strobe("OP_gm   : ",  OP_gm     ) ; // Transconductance
$strobe("OP_beta : ", OP_beta     ) ; // Current amplification
$strobe("OP_gout : ", OP_gout     ) ; // Output conductance

$strobe("OP_gmu  : ",  OP_gmu      ) ; // Feedback transconductance
$strobe("OP_rb   : ",  OP_rb      ) ; // Base resistance
$strobe("OP_rc   : ",  OP_rc       ) ; // Collector resistance
$strobe("OP_cbe  : ",   OP_cbe      ) ; // Base-emitter capacitance

$strobe("OP_cbc       : ", OP_cbc         ) ; // Base-collector capacitance
$strobe("OP_ft        : ", OP_ft          ) ; // Good approximation for cut-off frequency

$strobe("OP_iqs       : ", OP_iqs         ) ; // Current at onset of quasi-saturation
$strobe("OP_xiwepi    : ", OP_xiwepi      ) ; // Thickness of injection layer
$strobe("OP_vb2c2star : ", OP_vb2c2star   ) ; // Physical value of internal base-collector bias

//self-heating
`ifdef SELFHEATING
$strobe("OP_pdiss : ", OP_pdiss       ) ; // Dissipation
`endif

$strobe("OP_tk : ", OP_tk         ) ; // Actual temperature

`endif
end
