module net_declare (out, in1, in2);

output out; 
input in1, in2;

assign out = in1 & in2; // No delay in a continuous assignment

endmodule

module stimulus;

wire #10 OUT; //Declare the delay on the net.
reg IN1, IN2;
initial
begin
	IN1 = 0; IN2= 0;
	#20 IN1=1; IN2= 1;

	#40 IN1 = 0;

	#40 IN1 = 1;
	#5 IN1 = 0;
	#150 $stop;
end

initial
//	$gr_waves("out", OUT, "in1", IN1, "in2", IN2);
	$monitor($time, "out", OUT, "in1", IN1, "in2", IN2);

net_declare rd1(OUT, IN1, IN2);

endmodule
