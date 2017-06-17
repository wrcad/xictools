module stimulus;

reg [3:0] A, B;
wire A_GT_B, A_LT_B, A_EQ_B;

//Instantiate the magnitude comparator
magnitude_comparator MC(A_GT_B, A_LT_B, A_EQ_B, A, B);

initial
	$monitor($time," A = %b, B = %b, A_GT_B = %b, A_LT_B = %b, A_EQ_B = %b",
				A, B, A_GT_B, A_LT_B, A_EQ_B);

//stimulate the magnitude comparator.
initial
begin
	A = 4'b1010; B = 4'b1001;
	# 10 A = 4'b1110; B = 4'b1111;
	# 10 A = 4'b0000; B = 4'b0000;
	# 10 A = 4'b1000; B = 4'b1100;
	# 10 A = 4'b0110; B = 4'b1110;
	# 10 A = 4'b1110; B = 4'b1110;
end

endmodule
