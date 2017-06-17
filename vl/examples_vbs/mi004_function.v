/*
 * Test function definition.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	blocking assignment
 */

module main;

	reg [0:3] a, main_a;

function [0:3] my_func;
	input [0:3] a, input_a; /* To test scope resolution. */
	reg [0:3] c, func_c;

	begin
	$write("entering my_func:\n");
	$write("a = %b, input_a = %b\n", a, input_a);
	$write("c = %b, func_c = %b\n", c, func_c);
	c = a + 4;
	func_c = input_a + 5;
	$write("leaving my_func:\n");
	$write("a = %b, input_a = %b\n", a, input_a);
	$write("c = %b, func_c = %b\n", c, func_c);
	my_func = 5;	
	end

endfunction

	initial
		begin
		a = 0;
		main_a = 0;
		$write("a = %b, main_a = %b\n", a, main_a);
		$write("result = %b (5)\n",
			my_func(a,main_a));
		$write("a = %b, main_a = %b\n", a, main_a);
		end

endmodule
