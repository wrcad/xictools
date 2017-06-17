//Define a 2-to-1 mux using switches 
module my_mux (out, s, i0, i1);

output out;
input s, i0, i1;

//internal wire
wire sbar; //complement of s

//create the complement of s
my_nor nt(sbar, s, s); //equivalent to a not gate

//instantiate cmos switches
cmos (out, i0, sbar, s);
cmos (out, i1, s, sbar);

endmodule

//stimulus to test the gate
module  stimulus;
reg S, I0, I1;
wire OUT;


//instantiate the my_mux module
my_mux  m1(OUT, S, I0, I1); 

//Apply stimulus 
initial
begin
		//first combination
		I0 = 1'b1; I1 = 1'b0;
		S = 1'b0;
		#5 S = 1'b1;
		
		//second combination
		#5 I0 = 1'b0; I1 = 1'b1;
	  S = 1'b0;
		#5 S = 1'b1;
end

//check results
initial
		$monitor($time,"  OUT= %b, S= %b I0= %b, I1= %b",OUT,S,I0,I1);

endmodule

//module my_nor defined earlier
module my_nor(out, a, b);

output out;
input a, b;

//internal wires
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

