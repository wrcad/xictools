
module counter;

integer count;

initial
	for ( count=0; count < 128; count = count + 1)
			$display("Count = %d", count);

endmodule

