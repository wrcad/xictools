module latch(Q, d, clock, reset);

output Q;
input d, clock, reset;

reg q;

assign Q = q;

//A level sensitive latch with asynchronous reset 
always @( reset or clock or d) 
//Wait for reset or clock or d to change
begin 
        if (reset) 							 //if reset signal is high, set q to 0. 
                q = 1'b0; 
        else    if(clock)       //if clock is high, latch input  
                q = d; 
end 

endmodule
 

