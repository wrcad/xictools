module clock_gen;
//Example 1: Clock generation 
//Use forever loop instead of always block
reg clock;

initial
begin
        clock = 1'b0;
        forever #10 clock = ~clock; //Clock with period of 20 units
end

initial
       #100000 $finish;

endmodule

