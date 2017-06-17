
//Define a module called operation which contains the task bitwise_oper
module operation;
parameter delay = 10;
reg [15:0] A, B;
reg [15:0] AB_AND, AB_OR, AB_XOR;

initial
	$monitor( "%0d AB_AND = %b, AB_OR = %b, AB_XOR = %b, A = %b, B = %b",
                $time, AB_AND, AB_OR, AB_XOR, A, B);
		
initial
begin
	#1 A = 16'b1111_0000_1010_0111;
	B = 16'b1010_0101_1000_1100;
end

always @(A or B) //whenever A or B changes in value
begin
        //invoke the task bitwise_oper. provide 2 input arguments A, B
        //Expect 3 output arguments AB_AND, AB_OR, AB_XOR
	//The arguments must be specified in the same order as they
	//appear in the task declaration.
        bitwise_oper(AB_AND, AB_OR, AB_XOR, A, B);
end


//define task bitwise_oper
task bitwise_oper;
output [15:0] ab_and, ab_or, ab_xor; //outputs from the task
input [15:0] a, b; //inputs to the task
begin
        #delay ab_and = a & b;
        ab_or = a | b;
        ab_xor = a ^ b;
end
endtask
endmodule
