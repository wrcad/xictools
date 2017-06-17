module find_true_bit;

//Illustration: Find the first bit with a value 1 in flag (vector variable)
reg [15:0] flag;
integer i; //integer to keep count
 
initial
begin
  flag = 16'b 0010_0000_0000_0000;
  i = 0;
  begin: block1 //The main block inside while is named block1
  	while(i < 16) 
		begin
     if (flag[i])
     begin
        $display("Encountered a TRUE bit at element number %d", i);
        disable block1; //disable block1 because you found true bit.
     end
     i = i + 1;
		end
  end
end

endmodule
