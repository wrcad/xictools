//Define a time scale for the module dummy1
//Reference time unit is 100  nano second and precision is 1 ns
`timescale 100 ns / 1 ns

module dummy1;

reg toggle;

//initialize toggle
initial
begin
	toggle = 1'b0;
        #100 $finish;
end

//Flip the toggle register every 5 time units
//In this module 5 time units = 500 ns = .5 us
always #5 
begin
	toggle = ~toggle;
	$display("%d , In %m toggle = %b ", $time, toggle);
end

endmodule

//Define a time scale for the module dummy2
//Reference time unit is 1 micro second and precision is 10 ns
`timescale 1 us / 10 ns

module dummy2;
 
reg toggle;
 
//initialize toggle 
initial  
  toggle = 1'b0; 
 
//Flip the toggle register every 5 time units 
//In this module 5 time units = 5 us  = 5000 ns 
always #5 
begin
        toggle = ~toggle;
        $display("%d , In %m toggle = %b ", $time, toggle); 
    end 
 
endmodule


