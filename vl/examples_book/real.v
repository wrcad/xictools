module real_example;

real delta; // Define a real variable called delta
initial
begin
	delta = 4e10; // delta is assigned in scientific notation
	delta = 2.13; // delta is assigned a value 2.13
end

integer i; // Define an integer i
initial
        begin
	i = delta; // i gets the value 2 (rounded value of 2.13)
        $write(delta, i);
        end

endmodule

