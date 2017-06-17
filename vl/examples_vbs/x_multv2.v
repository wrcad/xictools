/*
 * Multiply module
 *  dependencies:
 *	register declaration
 *	io declaration
 *	always procedural block
 *	system tasks
 *	ifelse statement
 */
/*
 * Multiplier from Hennessy [94].  First iteration.
 */

module multiply (clk, prod, fin, mcand, mplier, reset);
	input clk, reset;
	output [63:0] prod;
	output fin;
	input [31:0] mcand, mplier;

	reg [63:0] prod;
	reg [7:0] working;
	reg fin;

	always @(posedge clk)
		begin
		if (reset == 1)
			begin
			fin = 0;
			prod = 0;
			working = 1;	// Start work on next clk!
			end
		else
			begin
			if (working == 1)
				begin
				// Make copies of the data.
				prod[31:0] = mplier;
				working = working + 1;
				end
			else if (working > 1)
				begin
				if (prod[0:0] == 1)
					prod[63:32] = prod[63:32] + mcand;
				prod = prod >> 1;

				// When are we finished?
				if (working > 32)
					fin = 1;
				working = working + 1;
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

module multiple_test ;
	reg [31:0] mplier, mcand;
	reg clk, reset;
	wire [63:0] product;
	wire finished;

	multiply m(clk, product, finished, mcand, mplier, reset);

	initial
		begin
		clk = 0;
		mcand = 14;
		mplier = 13;
		reset = 0;
		#1 reset = 1;
		#4 reset = 0;	// Must set reset for one clock cycle.
		end

	always #1 clk = !clk;

	always @(posedge finished)
		begin
		$write("Finished at %d:  %d * %d = %d\n",
			$time, mcand, mplier, product);
		$finish;
		end
endmodule
