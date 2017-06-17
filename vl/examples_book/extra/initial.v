
module stimulus;

reg x,y, a,b, m;

initial
	m = 1'b0; //single statement; does not need to be grouped

initial
begin
	#5 a = 1'b1; //multiple statements; need to be grouped
	#25 b = 1'b0;
end

initial
begin
	#10 x = 1'b0;
	#25 y = 1'b1;
end

initial
	#50 $finish;

endmodule


