//Conditional Compilation

//Example 1
`ifdef TEST //compile the module test only if the flag TEST is set
module test;
	initial
		$display("Module %m compiled");
endmodule
`else //compile the module stimulus as default
module stimulus;
	initial
		$display("Module %m compiled");
endmodule
`endif //completion of `ifdef statement

//Example 2
module top;

bus_master b1(); //instantiate module unconditionally
`ifdef ADD_B2
	bus_master b2(); //b2 is instantiated conditionally if flag ADD_B2 
								 //is set
`endif

endmodule


//define module with delays
module bus_master;
initial
	$display("Module %m instantiated");

endmodule


