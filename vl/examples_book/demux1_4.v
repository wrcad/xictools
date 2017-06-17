
module demultiplexer1_to_4 (out0, out1, out2, out3, in, s1, s0);  
 
// Port declarations from the I/O diagram
output out0, out1, out2, out3;
reg  out0,  out1,  out2,  out3;
input in;
input s1, s0;



always @(s1 or s0 or in)
case ({s1, s0}) //Switch based on control signals
    2'b00 :  begin  out0 = in;  out1 = 1'bz;  out2 = 1'bz;  out3 = 1'bz; end
    2'b01 :  begin  out0 = 1'bz;  out1 = in;  out2 = 1'bz;  out3 = 1'bz; end
    2'b10 :  begin  out0 = 1'bz;  out1 = 1'bz;  out2 = in;  out3 = 1'bz; end
    2'b11 :  begin  out0 = 1'bz;  out1 = 1'bz;  out2 = 1'bz;  out3 = in; end

    //Account for unknown signals on select. If any select signal is x 
    //then outputs are x. If any select signal is z, outputs are z. If one
    //is x and the other is z, x gets higher priority.
    2'bx0, 2'bx1, 2'bxz, 2'bxx, 2'b0x, 2'b1x, 2'bzx : 
         begin
             	 out0 = 1'bx;  out1 = 1'bx;  out2 = 1'bx;  out3 = 1'bx;
         end
    2'bz0, 2'bz1, 2'bzz, 2'b0z, 2'b1z :   
				 begin 
               out0 = 1'bz;  out1 = 1'bz;  out2 = 1'bz;  out3 = 1'bz;
         end
    default: $display("Unspecified control signals");
endcase 
 
endmodule 
 

module stimulus;
wire OUT0, OUT1, OUT2, OUT3;
reg IN, S1, S0;

//instantiate the decoder
demultiplexer1_to_4 dm0( OUT0, OUT1, OUT2, OUT3,  IN, S1, S0);

initial
$monitor($time,"OUT0 = %b,OUT1= %b,OUT2= %b,OUT3= %b,IN= %b,S1= %b,S0= %b ",
										OUT0, OUT1, OUT2, OUT3,  IN, S1, S0);

initial
begin
	#5	IN = 1;	S1 = 0; S0 = 0;
	#5	IN = 1;	S1 = 0; S0 = 1;
	#5	IN = 1;	S1 = 1; S0 = 0;
	#5	IN = 1;	S1 = 1; S0 = 1;

	#5	IN = 1;	S1 = 1'bx; S0 = 0;
	#5	IN = 1;	S1 = 0; S0 = 1'bx;
	#5	IN = 1;	S1 = 1'bz; S0 = 0;
	#5	IN = 1;	S1 = 0; S0 = 1'bz;

	#5	IN = 1;	S1 = 1'bx; S0 = 1'bz;

end

endmodule
	
