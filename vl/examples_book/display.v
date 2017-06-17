
module display_example;
	reg [0:40] virtual_addr;
	reg [4:0] port_id;
	reg [3:0] bus;

initial
begin
	//Display the string in quotes
	$display("Hello Verilog World");
	
	#200;
	//Display value of 41-bit   virtual address 1fe0000001c and time 200
	virtual_addr = 41'h1fe0000001c;
	$display("At time %d virtual address is %h", $time, virtual_addr);
	
	#30;
	//Display value of current simulation time 230
	$display($time);
	
	//Display value of port_id 5 in binary
	port_id = 5;
	$display("ID of the port is %b", port_id);
	
	//Display x characters
	//Display value of 4-bit bus 10xx (signal contention) in binary
	bus = 4'b10xx;
	$display("Bus value is %b", bus);
	
	//Display the hierarchical name of the current instance 
	$display("This string is displayed from %m level of hierarchy");


	//Display special characters, newline and %
	$display("This is a \n multi-line string with a %% sign");

end

endmodule
