
module reg_example;

reg reset; // declare a variable reset which can hold its value
initial // this construct will be discussed later
begin
	reset = 1'b1; //initialize reset to 1 to reset the digital circuit.
	#100 reset = 1'b0; // after 100 time units reset is deasserted.
end

endmodule
