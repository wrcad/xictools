//Define our own nor gate, my_nor
module my_nor(out, a, b);

output out;
input a, b;

wire c;

//set up power and ground lines
supply1 pwr;   	//pwr is connected to Vdd
supply0 gnd;	//gnd is connected to Vss(ground)

//instantiate pmos  switches
pmos	(c, pwr, b); 
pmos  (out, c, a);

//instantiate nmos switches
nmos  (out, gnd, a);
nmos  (out, gnd, b);

endmodule

//stimulus to test the gate
module  stimulus;
reg A, B;
wire OUT;

//instantiate the my_nor module
my_nor  n1(OUT, A, B); 

//Apply stimulus 
initial
begin
		//test all possible combinations
		A = 1'b0;  B = 1'b0;
		#5 A = 1'b0;  B = 1'b1;
		#5 A = 1'b1;  B = 1'b0;
		#5 A = 1'b1;  B = 1'b1;
end

//check results
initial
		$monitor($time, "  OUT = %b, A = %b, B = %b", OUT, A, B);

endmodule

