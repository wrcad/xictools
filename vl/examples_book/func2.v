
//Define a module which contains the function shift
module shifter;

//Left/right shifter
`define LEFT_SHIFT      1'b0
`define RIGHT_SHIFT     1'b1
reg [31:0] addr, left_addr, right_addr;
reg control;

initial
begin
	$monitor("%0d left= %h right = %h addr = %h",
			$time, left_addr, right_addr, addr);

        #1 addr = 32'h3456_789a;
        #10 addr = 32'hc4c6_78ff;
        #10 addr = 32'hff56_ff9a;
        #10 addr = 32'h3faa_aaaa;
end

 
//Compute the right and left shifted values whenever
//a new address value appears
always @(addr)
begin
	//call the function defined below to do left and right shift.
        left_addr = shift(addr, `LEFT_SHIFT);
        right_addr = shift(addr, `RIGHT_SHIFT);
end


//define shift function. The output is a 32-bit value.
function [31:0] shift;
input [31:0] address;
input control;
begin
        //set the output value appropriately based on a control signal. 
        shift = (control == `LEFT_SHIFT) ?(address << 1) : (address >> 1);
                        
end
endfunction


endmodule

