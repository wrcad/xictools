//Define an edge sensitive sequential UDP
primitive edge_dff(q, d, clock, clear);

//Declarations
output q;
reg q;
input d, clock, clear;

//sequential initialization
initial
  q = 0;

table
    //  d  clock  clear  :  q  :  q+  ;

        ?    ?      1    :  ?  :  0 ; //output = 0 if clear = 1
        ?    ?      f    :  ? :  - ; //ignore negative transition of clear

        1    f      0    :  ?  :  1 ; //latch data on negative transition of
        0    f      0    :  ?  :  0 ; //clock

        ?    (1x)  0    :  ?  : - ; //hold q if clock transitions to unknown
                                //state
        
        ?    p      0   :  ?  : - ; //ignore positive transitions of clock

        *    ?      0    :  ?  : - ; //ignore any change in d when clock is steady
endtable

endprimitive
      
// Edge triggered T-flipflop. Toggles every clock
// cycle.
module T_ff(q, clk, clear);

// I/O ports
output q;
input clk, clear;

// Instantiate the edge triggered DFF
// Complement of output q is fed back.
// Notice qbar not needed. Empty port.
edge_dff ff1(q, ~q, clk, clear);

endmodule

// Ripple counter
module counter(Q , clock, clear);

// I/O ports
output [3:0] Q;
input clock, clear;
 
// Instantiate the T flipflops
T_ff tff0(Q[0], clock, clear);
T_ff tff1(Q[1], Q[0], clear);
T_ff tff2(Q[2], Q[1], clear);
T_ff tff3(Q[3], Q[2], clear);
 
endmodule

// Top level stimulus module
module stimulus;
 
// Declare variables for stimulating input
reg CLOCK, CLEAR;
wire [3:0] Q;
 
initial
  $monitor($time, " Count Q = %b Clear= %b",  Q[3:0],CLEAR);
 
/*
initial
  $gr_waves(  "clk", CLOCK,
      "Clear", CLEAR,
      "Q", Q[3:0],
      "Q0", Q[0],
      "Q1", Q[1],
      "Q2", Q[2],
      "Q3", Q[3]);
*/
 
// Instantiate the design block counter
counter c1(Q, CLOCK, CLEAR);
 
// Stimulate the Clear Signal
initial
begin
  CLEAR = 1'b1;
  #34 CLEAR = 1'b0;
  #200 CLEAR = 1'b1;
  #50 CLEAR = 1'b0;
end
 
// Setup the clock to toggle every 10 time units
initial
begin
  CLOCK = 1'b0;
  forever #10 CLOCK = ~CLOCK;
end
 
// Finish the simulation at time 200
initial
begin
  #400 $stop;
end
 
endmodule

      
        
        



