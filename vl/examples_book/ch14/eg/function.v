module fadder (c_out, sum, a, b, c_in);

output c_out; 
output [3:0] sum;
input [3:0] a, b; 
input c_in;

assign {c_out, sum} = fulladd( a, b, c_in);

function [4:0] fulladd;
input [3:0] a, b;
input c_in;

begin
	fulladd = a + b + c_in;
end
endfunction

endmodule

