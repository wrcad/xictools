/*
 * Test if/else statement.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;
	reg [1:4] a;

	initial
		begin
		if (0)
			$write("incorrect, passed a false test.\n");
		else
			$write("correct, failed a false test.\n");
		if (1)
			$write("correct, passed a true test.\n");
		else
			$write("incorrect, failed a true test.\n");
		a = 0;
		if (a)
			$write("incorrect, a = %h (0x0).\n", a);
		else
			$write("correct, a = %h (0x0).\n", a);
		a = 4'hf;
		if (a)
			$write("correct, a = %h (0xf)\n", a);
		else
			$write("incorrect, a = %h (0xf).\n", a);
		a = 6;
		if (a[2:3])
			$write("correct, a[2:3] = %h (0x3)\n", a[2:3]);
		else
			$write("incorrect, a[2:3] = %h (0x3).\n", a[2:3]);
		if (a[4:4])
			$write("incorrect, a[4:4] = %h (0x0)\n", a[4:4]);
		else
			$write("correct, a[4:4] = %h (0x0)\n", a[4:4]);
		$finish;
		end

endmodule
