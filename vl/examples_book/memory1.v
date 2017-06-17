module test;

reg [7:0] memory[0:7]; //declare an 8 bit memory with 8 locations
integer i;

initial 
begin
	//read memory file. address locations given in memory
	$readmemh("init1.dat", memory);
	//display contents of initialized memory
	for(i=0; i < 8; i = i + 1)
		$display("Memory [%0d] = %b", i, memory[i]);
end

endmodule



