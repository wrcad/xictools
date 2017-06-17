module stop_finish;

reg clock, reset;

// Stop at time 100 in the simulation and examine the results
// Finish the simulation at time.
initial // to be explained later. time = 0
begin
	clock = 0;
	reset = 1;
	#100 $stop; // This will suspend the simulation at time = 100
	#900 $finish; // This will finish the simulation at time = 1000
end

endmodule
