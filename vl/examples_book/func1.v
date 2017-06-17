//Define a module which contains the function calc_parity
module parity;
reg [31:0] addr;
reg parity;

initial
begin
	addr = 32'h3456_789a;
	#10 addr = 32'hc4c6_78ff;
	#10 addr = 32'hff56_ff9a;
	#10 addr = 32'h3faa_aaaa;
end

//Compute new parity whenever address value changes
always @(addr)
begin
        parity = calc_parity(addr); //First invocation of calc_parity
        $display("Parity calculated = %b", calc_parity(addr) ); 
                                    //Second invocation of calc_parity
end

//define the parity calculation function
function calc_parity;
input [31:0] address;
begin
        //set the output value appropriately. Use the implicit
        //internal register calc_parity.
        calc_parity = ^address; //Return the ex-or of all address bits.
end
endfunction

endmodule


