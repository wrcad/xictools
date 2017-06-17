module find_true_bit;

//Illustration 2: Find the first bit with a value 1 
//in flag (vector variable)
`define TRUE 1'b1
`define FALSE 1'b0
reg [15:0] flag;
integer i; //integer to keep count
reg continue;

initial
begin
  flag = 16'b 0010_0000_0000_0000;
  i = 0;
  continue = `TRUE;

  while((i < 16) && continue ) //Multiple conditions using operators.
  begin
    if (flag[i])
    begin
      $display("Encountered a TRUE bit at element number %d", i);
      continue = `FALSE;
 	end
 	i = i + 1;
  end
end
 
endmodule
