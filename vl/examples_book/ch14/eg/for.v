module fadder8bit ( c_out, sum, a, b, c_in);

output c_out;
reg c_out;
output [7:0] sum;
reg [7:0] sum;

input [7:0] a, b;
input c_in;

reg c;

integer i;

always @(a or b or c_in)
begin
c = c_in;
for (i=0; i <=7; i = i+1)
	{c, sum[i] } = a[i] + b[i] + c;

c_out <= c;
end
endmodule


