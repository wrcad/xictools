
module sequential;

//Illustration 2: Sequential blocks with delay.
reg x, y;
reg [1:0] z, w;

initial
        $monitor($time, " x = %b, y = %b, z = %b, w = %b\n",
											  x, y, z, w);

//Nested blocks
initial
begin
        x = 1'b0; 
        fork 
                #5 y = 1'b1; 
                #10 z = {x, y}; 
        join
        #20 w = {y, x};
end

endmodule
