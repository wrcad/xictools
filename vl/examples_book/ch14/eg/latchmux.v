//Latch is inferred because of incomplete specification
module incomplete (out, control, a);

output out;
reg out;
input control, a;

always @(control or a)
	if(control)
		out <= a;

endmodule

//Multiplexer is inferred because of complete specification
module complete (out, control, a, b);

output out;
reg out;
input control, a, b;

always @(control or a or b)
	if(control)
		out <= a;
	else
		out <= b;

endmodule
