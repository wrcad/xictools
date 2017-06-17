//define module with delays
module bus_master;
//default parameter values
parameter delay1 = 2;
parameter delay2 = 3;
parameter delay3 = 7;

initial
	$display("Module %m : delay1 = %d, delay2 = %d, delay3 = %d",
												delay1, delay2, delay3);

endmodule

//top level module; instantiates two bus_master modules
module top;

//Instantiate the modules with new delay values
bus_master #(4, 5, 6) b1(); //b1: delay1 = 4, delay2 = 5, delay3 = 6
bus_master #(9, 4) b2(); //b2: delay1 = 9, delay2 = 4, delay3 = 7(default)

endmodule

