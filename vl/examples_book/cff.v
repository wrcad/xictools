//Define an inverter using MOS switches
module my_not(out, in);

output out;
input in;

//define power and ground
supply1 pwr;
supply0 gnd;

//instantiate nmos and pmos switches
pmos  (out, pwr, in);
nmos  (out, gnd, in);

endmodule

//Define a CMOS flipflop
module cff ( q, qbar, d, clk);

output q, qbar;
input d, clk;

//internal nets
wire e;
wire nclk; //complement of clock

//instantiate the inverter
my_not nt(nclk, clk);

//instantiate CMOS switches
cmos  (e, d, clk, nclk); //switch C1; e = d when clk = 1.
cmos  (e, q, nclk, clk); //switch C2; e = q when clk = 0.


//instantiate the inverters
my_not nt1(qbar, e);
my_not nt2(q, qbar);

endmodule

//Test the flipflop using a stimulus block
module stimulus;

reg D, CLK;
wire Q, QBAR;

//instantiate the CMOS flipflop
cff c1(Q, QBAR, D, CLK);

//test load and store using stimulus
initial
begin
		//sequence 1
		CLK = 1'b0;
		D = 1'b1;
		#5 CLK = 1'b1; 						//flipflop will store value of D
		#5 CLK = 1'b0;  					//during this pulse

		//sequence 2
		#10 D = 1'b0;
		#5 CLK = 1'b1; 						//flipflop will store value of D
		#5 CLK = 1'b0;            //during this pulse
end

//check output
initial
		$monitor($time,"  CLK = %b, D = %b, Q = %b, QBAR = %b ", 
														CLK, D, Q, QBAR);

endmodule

		
		
