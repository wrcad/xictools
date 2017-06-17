//Conditional execution
module test;
reg a, b, c;
initial
begin
	a = 1'b1; b = 1'b0; c = 1'b1;
	if($test$plusargs("DISPLAY_VAR"))
 			$display("Display = %b ", {a,b,c} ); //display only if flag is set
	else
 			$display("No Display"); //otherwise no display
end
endmodule

