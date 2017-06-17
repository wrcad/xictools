module stimulus;

wire Q, Qbar;
reg D, CLK, RESET;

always #5 CLK = ~CLK;

initial
begin
	RESET = 1'B1;
	CLK = 1'B0;
	#10 RESET = 1'B0;

	D = 1'b0;
	#110 D = 1'b0;
        #150 D = 1'b1;	
	#100 $stop;
end

initial
	$gr_waves("Q", Q, "CLK", CLK, "RST", RESET, "D", D);
	
//instantiate the d-flipflop
edge_dff dff(Q, Qbar, D, CLK, RESET);
initial
begin
	//these statements force value of 1 on dff.q between time 50 and 100
	//regardless of what the actual output of dff is.
	#50 force dff.q = 1'b1; //force value of q to 1 at time 50. 
	#50 release dff.q;      //release the value of q 
end
endmodule

