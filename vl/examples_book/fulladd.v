primitive udp_and(out, a, b);

output out;
input a, b;

table
   //  a    b    :   out;
      0    0    :    0;
      0    1    :    0;
      1    0    :    0;
      1    1    :    1;
endtable

endprimitive

primitive udp_xor(out, a, b);

output out;
input a, b;

table
  //  a    b    :   out;
      0    0    :    0;
      0    1    :    1;
      1    0    :    1;
      1    1    :    0;
endtable

endprimitive

/*
primitive udp_or(out, a, b);

output out;
input a, b;

table
  //  a    b    :   out;
      0    0    :    0;
      0    1    :    1;
      1    0    :    1;
      1    1    :    1;
      x    0    :    x;
      x    1    :    1;
      0    x    :    x;
      1    x    :    1;
endtable
endprimitive
*/

primitive udp_or(out, a, b);

output out;
input a, b;

table
  //  a    b    :   out;
      0    0    :    0;
      1    ?    :    1;
      ?    1    :    1;
      0    x    :    x;
      x    0    :    x;
endtable

endprimitive

// Define a 1-bit full adder
module fulladd(sum, c_out, a, b, c_in);

// I/O port declarations
output sum, c_out;
input a, b, c_in;

// Internal nets
wire s1, c1, c2;

// Instantiate logic gate primitives
udp_xor (s1, a, b);
udp_and (c1, a, b);

udp_xor (sum, s1, c_in);
udp_and (c2, s1, c_in);

udp_or  (c_out, c2, c1);

endmodule


// Define a 4-bit full adder
module fulladd4(sum, c_out, a, b, c_in);

// I/O port declarations
output [3:0] sum;
output c_out;
input[3:0] a, b;
input c_in;

// Internal nets
wire c1, c2, c3;

// Instantiate four 1-bit full adders.
fulladd fa0(sum[0], c1, a[0], b[0], c_in);
fulladd fa1(sum[1], c2, a[1], b[1], c1);
fulladd fa2(sum[2], c3, a[2], b[2], c2);
fulladd fa3(sum[3], c_out, a[3], b[3], c3);

endmodule

// Define the stimulus (top level module)
module stimulus;

// Set up variables
reg [3:0] A, B;
reg C_IN;
wire [3:0] SUM;
wire C_OUT;
 
// Instantiate the 4-bit full adder. call it FA1_4
fulladd4 FA1_4(SUM, C_OUT, A, B, C_IN);
 
 
// Setup the monitoring for the signal values
initial
begin
  $monitor($time," A= %b, B=%b, C_IN= %b,, C_OUT= %b, SUM= %b\n",
                          A, B, C_IN, C_OUT, SUM);
end
 
// Stimulate inputs
initial
begin
  A = 4'd0; B = 4'd0; C_IN = 1'b0;
 
  #5 A = 4'd3; B = 4'd4;
 
  #5 A = 4'd2; B = 4'd5;
  
  #5 A = 4'd9; B = 4'd9;
  
  #5 A = 4'd10; B = 4'd15;
 
  #5 A = 4'd10; B = 4'd5; C_IN = 1'b1;
end
 
endmodule
