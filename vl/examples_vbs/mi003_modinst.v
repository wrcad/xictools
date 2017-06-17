/*
 * Test module instantiation.
 *  dependencies:
 *	port list
 *	all declarations
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	event expression
 */

module a_module (a, module_a, b, module_b);
	input a, module_a;
	output b, module_b;
	reg b, module_b;

	always @(negedge a)
		begin
		b = ~a;
		$write("a = %b b = %b\n", a, b);
		end

	always @(posedge a or negedge module_a)
		begin
		b = ~a;
		module_b = ~module_a;
		$write("a = %b module_a = %b b = %b module_b = %b\n",
			a, module_a, b, module_b);
		end

endmodule


module main;
	reg a1, main_a1, a2, main_a2;
	wire b1, main_b1, b2, main_b2;

	a_module a_mod1(a1, main_a1, b1, main_b1);
	a_module a_mod2(a2, main_a2, b2, main_b2);

	initial
		begin
		a1 = 0;
		main_a1 = 0;
		#1 a1 = 1;
		#1 a1 = 0;
		#1 main_a1 = 1;
		#1 main_a1 = 0;
		#1 a1 = 1;
		end

	initial
		begin
		a2 = 0;
		main_a2 = 0;
		#1 a2 = 1;
		#1 a2 = 0;
		#1 main_a2 = 1;
		#1 main_a2 = 0;
		#1 a2 = 1;
		end

	always @(b1)
		$write("%d: b1 = %b\n", $time, b1);

	always @(b2)
		$write("%d: b2 = %b\n", $time, b2);

	always @(a1 or main_a1 or b1 or main_b1)
		$write("%d: a1 = %b main_a1 = %b b1 = %b main_b1 = %b\n",
			$time, a1, main_a1, b1, main_b1);

	always @(a2 or main_a2 or b2 or main_b2)
		$write("%d: a2 = %b main_a2 = %b b2 = %b main_b2 = %b\n",
			$time, a2, main_a2, b2, main_b2);

endmodule
