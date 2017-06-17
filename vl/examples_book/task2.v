
//Define a module which contains the task asymmetric_sequence
module sequence;
reg clock;

initial
begin
//	$gr_waves("clock", clock);
	$monitor($time, "clock", clock);
	#100 $stop;
end

initial
        init_sequence; //Invoke the task init_sequence

always 
begin
        asymmetric_sequence; //Invoke the task asymmetric_sequence
end
 
 
//Initialization sequence
task init_sequence;
begin
        clock = 1'b0;
end
endtask

//define task to generate asymmetric sequence
//operate directly on the clock defined in the module.
task asymmetric_sequence;
begin
        #12 clock = 1'b0;
        #5 clock = 1'b1;
        #3 clock = 1'b0;
        #10 clock = 1'b1;
end
endtask
 
 
endmodule


