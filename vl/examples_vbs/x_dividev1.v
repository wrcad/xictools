/*
 * Divide module
 *  dependencies:
 *	register declaration
 *	io declaration
 *	always procedural block
 *	system tasks
 *	ifelse statement
 */

module divider (clk, quot, rem, fin, dvdend, dvsor, reset);
	output [31:0] quot;
	output [63:0] rem;
	output fin;
	input [63:0] dvdend;
	input [31:0] dvsor;
	input clk, reset;

	reg [31:0] quot;
	reg [63:0] rem, dvsor_copy;
	reg fin;
	reg [5:0] rep;

	always @(posedge clk)
		begin
		if (reset == 1)
			begin
			quot = 0;
			dvsor_copy = 0;
			fin = 0;
			rep = 0;
			end
		else
			begin
			if (rep == 0)
				begin
				// Make copies of the data.
				rem = dvdend;
				dvsor_copy[63:32] = dvsor;
				rep = rep + 1;
				end
			else if (rep >= 1)
				begin
				rem = rem - dvsor_copy;
				quot = quot << 1;
				if (rem[63:63] == 1)
					rem = dvsor_copy + rem;
				else
					quot[0:0] = 1;
				dvsor_copy = dvsor_copy >> 1;
				// When are we finished?
				if (rep == 33)
					fin = 1;
				rep = rep + 1;
				end
			end
		end

endmodule

/*
 * Complex test.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	module instantiation
 *	system tasks
 */

module divide_test ;
	reg [31:0] dvdend, dvsor;
	reg clk, reset;
	wire [31:0] quotient;
	wire [63:0] remainder;
	wire finished;

	divider div(clk, quotient, remainder, finished, dvdend, dvsor, reset);

	initial
		begin
		clk = 0;
		dvdend = 7;
		dvsor = 2;
		reset = 0;
		#1 reset = 1;
		#3 reset = 0;	// Must set reset for one clock cycle.
		end

	always #1 clk = ~clk;

	always @(posedge finished)
		begin
		$write("Finished at %d:  %d / %d = %d(%d,%d)\n",
			$time, dvdend, dvsor, quotient,
			remainder[31:0], remainder[63:32]);
		$finish;
		end

endmodule
