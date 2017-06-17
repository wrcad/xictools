module dummy;
reg x, y, z;
reg [15:0] reg_a, reg_b;
integer count;


//All behavioral statements must be inside an initial or always block
initial
begin
        x = 0; y = 1; z = 1; //Scalar assignments
        count = 0; //Assignment to integer variables
        reg_a = 16'b0; reg_b = reg_a; //initialize vectors

        reg_a[2] = #15 1; //Bit select assignment with delay
        reg_b[15:13] = #10 {x, y, z};//Assign result of concatenation to
                                     // part select of a vector
        count = count + 1; //Assignment to an integer (increment)
end

initial
        $monitor($time, 
        " x = %b, y = %b, z = %b, count = %0d, reg_a = %x, reg_b = %x",
				x, y, z, count, reg_a, reg_b);

endmodule
