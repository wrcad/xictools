module parallel;

//Illustration 1:Parallel Blocks with delay
reg x, y;
reg [1:0] z, w;

initial
        $monitor($time, " x = %b, y = %b, z = %b, w = %b\n",
											  x, y, z, w);

initial
fork
        x = 1'b0; //completes at simulation time 0
        #5 y = 1'b1; //completes at simulation time 5
        #10 z = {x, y}; //completes at simulation time 10
        #20 w = {y, x}; //completes at simulation time 20
join


endmodule
