module fadder (c_out, sum, a, b, c_in);

output c_out, sum;
input a, b, c_in;

assign {c_out, sum} = a + b + c_in;

endmodule
