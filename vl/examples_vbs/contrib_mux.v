module mux (sel, a, b, f);   
  input sel, a, b;  
  output f;  

always @(sel or a or b)
begin
  if(sel == 0) f = a; 
  else  f = b; 
end

endmodule

module main ;
wire sel, a, b, f;
wire finished;

mux muxa(sel, a, b, f);

initial 
begin
  finished = 0;
  sel = 0; a = 0; b = 0;
  #10 a = 1;
  #10 sel = 1;
  #10 b = 1;
  #40 finished = 1;
end

always #1 $write("sel => %d, a => %d, b => %d, f => %d\n",sel, a, b, f);
always @(finished) $finish;


endmodule
