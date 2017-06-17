module  mux (out, s, i1, i0);

output out;
input s, i1, i0;

assign out = (s) ? i1 : i0;

endmodule
