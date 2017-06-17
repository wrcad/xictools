/*
 * Test case statement.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a;

	initial
		begin
		a = 4;
		case (a)
			1, 2, 3: $write("selected 1, 2, 3 (Not Ok)\n");
			4, 5, 6: $write("selected 4, 5, 6 (Ok)\n");
			7, 8, 9: $write("selected 7, 8, 9 (Not Ok)\n");
			default: $write("selected default (Not Ok)\n");
		endcase
		case (10)
			1, 2, 3: $write("selected 1, 2, 3 (Not Ok)\n");
			4, 5, 6: $write("selected 4, 5, 6 (Not Ok)\n");
			7, 8, 9: $write("selected 7, 8, 9 (Not Ok)\n");
			default: $write("selected default (Ok)\n");
		endcase
		casez (8'b1z0z_11zz)
			8'b0z1z_11zz: $write("selected 1z0z_1zz1 (Not Ok)\n");
			8'b1z0z_11zz: $write("selected 1z0z_11zz (Ok)\n");
			8'b1z0z_1zz1: $write("selected 1z0z_1zz1 (Not Ok)\n");
			default: $write("selected default (Not Ok)\n");
		endcase
		casex (8'b1xz1_1010)
			8'b1100_1xxx: $write("selected 1100_1xxx (Not Ok)\n");
			8'b1110_1xxx: $write("selected 1110_1xxx (Not Ok)\n");
			8'b1001_101z: $write("selected 1001_101z (Ok)\n");
			default: $write("selected default (Not Ok)\n");
		endcase
		$finish;
		end

endmodule
