/*
 * Test task definition.
 *  dependencies:
 *	initial procedural block
 *	sequential block
 *	system task write
 *	blocking assignment
 */

module main;

	reg [0:3] a;

task my_task;
	inout [0:3] inout_a;
	reg [0:3] a;

	begin
	a = inout_a + 1;	
	inout_a = inout_a + 2;	
	end

endtask

	initial
		begin
		a = 4'b0101;
		$write("a = %h\n", a);
		my_task(a);
		$write("result = %h\n", a);
		end

endmodule
