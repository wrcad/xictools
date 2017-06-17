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


