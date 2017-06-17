/************************************************
*
* Simulation Model for FIFO 512 X 18 from
* Integrated Device Technology IDT72215LB-25
*  02-22-96 Version 1.10
*  (c) 1996 B Seiler  Circuit City / Patapsco West
*
*   Changes
*   02-15-96  bjs Added Timing tests
*   02-16-96  bjs Synchronize Empty and Full Flags
*   02-22-96  bjs changed debug_prt to integer
*
************************************************/

`timescale 1 ns / 10 ps ;

module fifo(
  q,
  ef_,
  ff_,
  hf_,
  paf_,
  pae_,
  d,
  rs_,
  rclk_,
  ren_,
  oe_,
  wclk_,
  wen_,
  ld_) ;

parameter WIDTH = 18 ;  // Data width in bits these FIFO's are 18 bits 
parameter PSIZE = 9 ;   // address pointer size in FIFO
                        // usually this is set to 9 bits for 512 words in FIFO
                        // for simple simulation I set it to 3 bits for 8 word deep FIFO
parameter MSIZE = (1 << PSIZE) ; // FIFO size should be 512

parameter TOHZ = 1 ;
parameter TOE = 15 ;

output [WIDTH - 1:0] q ; 
input  [WIDTH - 1:0] d ;
input rs_, rclk_, ren_, oe_, wclk_, wen_, ld_ ;
output ef_, ff_, hf_, paf_, pae_ ;

wire [WIDTH - 1:0] q ;


reg [WIDTH - 1:0] outreg ;
reg [10:0] q_reg, p_reg ; // 11 bits for AP9A415 512 word FIFO
reg [WIDTH - 1:0] memarr[0:MSIZE - 1] ;
reg [PSIZE - 1:0] inptr, outptr ;
reg [PSIZE:0] fullctr ;
reg ef_, ff_, hf_, paf_, pae_ ;
reg ldw_state, ldr_state ;
reg q_enable ;

// ********************************************************************
// * Timing Verification
// ********************************************************************
// * Debug and Error Printing Control
parameter debug_prt = 1 ; // if 1 then print errors

// ********************************************************************
// * Timing tests
// *  Note following defines are for IDT72215LB25 25 ns FIFO
// *  all times are in nano seconds
// ********************************************************************

`define Fs    40 // Clock Cycle Frequency (max)
`define Ta    15 // Data Access Time (max)
`define Toe   12 // Output Enable to Output Valid (max)
`define Tolz   0 // Output Enable to Output in Low-Z (min)
`define Tohz  12 // Output Enable to Output in High-Z (max)
`define Tclk  25 // Clock Cycle Time (min)
`define Tclkh 10 // Clock High Time (min)
`define Tclkl 10 // Clock Low Time (min)
`define Tds    6 // Data Setup Time (min)
`define Tdh    1 // Data Hold Time (min)
`define Tens   6 // Enable Setup Time (min) 
`define Tenh   1 // Enable Hold Time (min)
`define Trs   25 // Reset Pulse Width (min)
`define Trss  15 // Reset Set-up Time (min)
`define Twff  15 // Write Clock to Full Flag (max)
`define Tref  15 // Read Clock to Empty Flag (max)
`define Twel   7 // Minimum Write Enable Low Time (min)
`define Trel   7 // Minimum Read Enable Low Time (min)

integer
time_temp,
time_ren_negedge,
time_ren_posedge,
time_rclk_negedge,
time_rclk_posedge,
time_wen_negedge,
time_wen_posedge,
time_wclk_negedge,
time_wclk_posedge,
time_oe_negedge,
time_oe_posedge,
time_rs_negedge,
time_rs_posedge,
time_wclk_last_posedge,
time_data_in_change ;


initial
  begin
    time_ren_negedge = -9999 ;
    time_ren_posedge = -9999 ;
    time_rclk_negedge = -9999 ;
    time_rclk_posedge = -9999 ;
    time_wen_negedge = -9999 ;
    time_wen_posedge = -9999 ;
    time_wclk_negedge = -9999 ;
    time_wclk_posedge = -9999 ;
    time_wclk_last_posedge = -9999 ;
    time_oe_negedge = -9999 ;
    time_oe_posedge = -9999 ;
    time_rs_negedge = -9999 ;
    time_rs_posedge = -9999 ;
    time_data_in_change = -9999 ;
  end

// ********************************************************************
// * Data In Timing
always @(d)
  begin
    time_temp = $time - time_wclk_posedge ;
    if (debug_prt && !wen_ && (time_temp < `Tdh))
      $display("FIFO... %10d ****** Error Write Data Hold too Short Tdh=%0d",
        $time, time_temp) ;

    time_data_in_change = $time ;
  end
  
// ********************************************************************
// * Write Timing
always @(posedge wclk_)
  begin
    time_temp = $time - time_wclk_posedge ;
    if (debug_prt && (time_temp < `Tclk))
      $display("FIFO... %10d ****** Error Write Clock (wclk_) Too Fast Tclk=%0d",
        $time, time_temp) ;

    time_temp = $time - time_wclk_negedge ;
    if (debug_prt && (time_temp < `Tclkl))
      $display("FIFO... %10d ****** Error Write Clock (wclk_) Low is too Short Tclkl=%0d",
        $time, time_temp) ;

    time_temp = $time - time_data_in_change ;
    if (debug_prt && !wen_ && (time_temp < `Tds))
      $display("FIFO... %10d ****** Error Write Data Set-up too Short Tds=%0d",
        $time, time_temp) ;

    time_temp = $time - time_wen_negedge ;
    if (debug_prt && !wen_ && (time_temp < `Tens))
      $display("FIFO... %10d ****** Error Write Enable (wen_) Set-up too Short Tens=%0d",
        $time, time_temp) ;

    time_wclk_posedge = $time ;
  end

always @(negedge wclk_)
  begin
    time_temp = $time - time_wclk_posedge ;
    if (debug_prt && (time_temp < `Tclkh))
      $display("FIFO... %10d ****** Error Write Clock (wclk_) High is too Short Tclkh=%0d",
        $time, time_temp) ;

    time_wclk_negedge = $time ;
  end
  
always @(negedge wen_)
  time_wen_negedge = $time ;

always @(posedge wen_)
  begin
    time_temp = $time - time_wclk_posedge ;
    if (debug_prt && (time_temp < `Tenh))
      $display("FIFO... %10d ****** Error Write Enable (wen_) Hold Time is too Short Tenh=%0d",
        $time, time_temp) ;

    time_temp = $time - time_wen_negedge ;
    if (debug_prt && (time_temp < `Twel))
      $display("FIFO... %10d ****** Error Write Enable (wen_) Low Time is too Short Twel=%0d",
        $time, time_temp) ;

    time_wen_posedge = $time ;
  end

// ********************************************************************
// * Read Timing
always @(posedge rclk_)
  begin
    time_temp = $time - time_rclk_posedge ;
    if (debug_prt && (time_temp < `Tclk))
      $display("FIFO... %10d ****** Error Read Clock (rclk_) Too Fast Tclk=%0d",
        $time, time_temp) ;

    time_temp = $time - time_rclk_negedge ;
    if (debug_prt && (time_temp < `Tclkl))
      $display("FIFO... %10d ****** Error Read Clock (rclk_) Low is too Short Tclkl=%0d",
        $time, time_temp) ;

    time_temp = $time - time_ren_negedge ;
    if (debug_prt && !ren_ && (time_temp < `Tens))
      $display("FIFO... %10d ****** Error Read Enable (ren_) Set-up too Short Tens=%0d",
        $time, time_temp) ;

    time_rclk_posedge = $time ;
  end

always @(negedge rclk_)
  begin
    time_temp = $time - time_rclk_posedge ;
    if (debug_prt && (time_temp < `Tclkh))
      $display("FIFO... %10d ****** Error Read Clock (rclk_) High is too Short Tclkh=%0d",
        $time, time_temp) ;

    time_rclk_negedge = $time ;
  end
  
always @(negedge ren_)
  time_ren_negedge = $time ;

always @(posedge ren_)
  begin
    time_temp = $time - time_rclk_posedge ;
    if (debug_prt && (time_temp < `Tenh))
      $display("FIFO... %10d ****** Error Read Enable (ren_) Hold Time is too Short Tenh=%0d",
        $time, time_temp) ;

    time_temp = $time - time_ren_negedge ;
    if (debug_prt && (time_temp < `Trel))
      $display("FIFO... %10d ****** Error Read Enable (ren_) Low Time is too Short Trel=%0d",
        $time, time_temp) ;

    time_ren_posedge = $time ;
  end

// ***********************************************************
// * Reset to FIFO
always @(negedge rs_)
  begin
    inptr = 0 ;          // start input and output pointer at beginning
    outptr = 0 ;
    fullctr = 0 ;        // FIFO is empty
    outreg = 0 ;
    q_reg = (MSIZE/8) - 1 ;
    p_reg = (MSIZE/8) - 1 ;
    ef_  <= 1'b0 ;
    pae_ <= 1'b0 ;
    hf_  <= 1'b1 ;
    ff_  <= 1'b1 ;
    paf_ <= 1'b1 ;
    ldw_state = 1'b0 ;
    ldr_state = 1'b0 ;
    q_enable = 1'b0 ;
  end

// ***********************************************************
// * Write to FIFO and writes to pae or paf registers
always @(posedge wclk_)
  begin
    if (!wen_ && rs_)
      begin
//        $display("FIFO... %10d got wen_=%d ld_=%d ff_=%d ", $time, wen_, ld_, ff_) ;
        if (ld_ && ff_)
          begin
//            $display("FIFO... %10d FIFO Write", $time) ;
            memarr[inptr] = d ;
            inptr = inptr + 1 ;
            fullctr = fullctr + 1 ;
          end
        else if (!ld_)  
          case (ldw_state)
            1'b0:
              begin
                q_reg = d[10:0] ;
                ldw_state = 1'b1 ;
//                $display("FIFO... %10d got q_reg=%d", $time, q_reg) ;
              end
            1'b1:
              begin
//                $display("FIFO... %10d got p_reg=%d", $time, p_reg) ;
                p_reg = d[10:0] ;
              end
          endcase
      end
  end
  
always @(negedge wen_)
  ldw_state = 1'b0 ;
  
      
// ***********************************************************
// * Reads from FIFO and reads of pae or paf registers
always @(posedge rclk_)
  begin
    if (!ren_ && rs_)
      if (ld_ && ef_)
        begin
          #`Ta outreg = memarr[outptr] ; // 15 ns
          fullctr = fullctr - 1 ;
          outptr = outptr + 1 ;
        end
      else if (!ld_)
        case (ldr_state)
          1'b0:
            begin
              #`Ta outreg = {{7{1'b0}}, q_reg} ;
              ldr_state = 1'b1 ;
            end
          1'b1:
              #`Ta outreg = {{7{1'b0}}, p_reg} ;
        endcase

  end

always @(negedge ren_)
  ldr_state = 1'b0 ;
  
// ***********************************************************
// * OE control
assign q = q_enable ? outreg : 18'bz ;

always @(negedge oe_)
  #`Tolz q_enable = 1'b1 ;

always @(posedge oe_)
  #`Tohz q_enable = 1'b0 ;

// ***********************************************************
// * Status flags for FIFO
reg temp_ef_ ;  // holding value for empty flag
reg temp_ff_ ;  // holding value for empty flag

always @(fullctr)
  begin
    temp_ff_  = ~fullctr[PSIZE] ;           // FIFO is full if overflow bit set
    temp_ef_  = |fullctr ;                  // FIFO is empty if fullctr = 0
    hf_  = ~(|fullctr[PSIZE:(PSIZE - 1)]) ; // FIFO is half full if overflow or MSB - 1 is set
    pae_ = (fullctr < (q_reg + 1)) ? 1'b0 : 1'b1 ; 
    paf_ = ((MSIZE - fullctr) <= (p_reg + 1)) ? 1'b0 : 1'b1 ; 
  end

// ***********************************************************
// * Synchronize Empty Flag with Read Clock
always @(posedge rclk_)
    #`Tref ef_  = temp_ef_ ;

// ***********************************************************
// * Synchronize Full Flag with Write Clock
always @(posedge wclk_)
    #`Twff ff_  = temp_ff_ ;
  

endmodule
  
