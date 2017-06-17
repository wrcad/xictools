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

//initial
//	$gr_waves("Q", Q, "CLK", CLK, "RST", RESET, "D", D);
initial
	$monitor($time, "Q", Q, "CLK", CLK, "RST", RESET, "D", D);
	
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

// Positive edge triggered D flipflop with asynchronous reset
module edge_dff(q, qbar, d, clk, reset);

// Inputs and outputs
output q,qbar;
input d, clk, reset;    
reg q, qbar; //declare q and qbar are registers

always @(negedge clk) //assign value of q and qbar at active edge of clock
begin
        q = d;
        qbar = ~d;
end

always @(reset) //Override the regular assignments to q and qbar
                 //whenever reset goes high. Use of procedural continuous
                 //assignments.
        if(reset) 
        begin   //if reset is high, override regular assignments to q with
                //the new values using procedural continuous assignment.
                assign q = 1'b0;
                assign qbar = 1'b1;
        end
        else
        begin   //if reset goes low, remove the overriding values by
                //deassigning the registers. After this the regular
                //assignments q = d and qbar = ~d will be able to change
		//the registers on the next negative edge of clock.
                deassign q; 
                deassign qbar; 
        end 
 
endmodule 


