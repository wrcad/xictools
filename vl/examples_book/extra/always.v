module clock_gen; 

reg clock;

//Initialize clock at time zero
initial
        clock = 1'b0;

//Toggle clock every half cycle (time period = 20)
always
        #10 clock = ~clock;

initial
        #1000 $finish;

endmodule


