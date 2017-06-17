/*
 * Test blocking assignment.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 */

module main;

	reg [0:7] a, a_long_variable_name;
	reg [3:0] b;
	reg c;
	reg [1:8] d;

	initial
		begin
		$write("%b %b %b %b (x x x x)\n", a, b, c, d);
		$write("long variable name = %b (x)\n", a_long_variable_name);
		a_long_variable_name = 8'b10000001;
		a = 255;
		b = 15;
		c = 0;
		d = 255;
		$write("%h %h %h %h (ff f 0 ff)\n", a, b, c, d);
		$write("long variable name = %h (81)\n", a_long_variable_name);
		a_long_variable_name[2:5] = b;
		a = 8'hcc;
		b = 4'o14;
		c = 1'b1;
		d = 8'h66;
		$write("%h %h %h %h (cc c 1 66)\n", a, b, c, d);
		$write("long variable name = %h (bd)\n", a_long_variable_name);
		a_long_variable_name = a[4:7];
		a[0:3] = b[1:0];
		b = d;
		d[4:4] = c;
		$write("%h %h %h %h (c0(should be 0c) 6 1 76)\n", a, b, c, d);
$write("a = %b, vbs-1.3.7 is wrong!\n", a);
		$write("long variable name = %h (0c)\n", a_long_variable_name);
		$write("%h %h %h %h (0(should be 6) 1 1 7)\n", a[3:6], b[3:2], c, d[1:4]);
		$finish;
		end

endmodule
