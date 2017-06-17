//Design the newspaper vending machine coin acceptor
//using a FSM approach
module vend( coin, clock, reset, newspaper);

//Input output port declarations
input [1:0] coin;             
input clock;
input reset;
output newspaper;                   
wire newspaper;

//internal FSM state declarations
wire [1:0] NEXT_STATE;
reg [1:0] PRES_STATE;

//state encodings
parameter s0 = 2'b00;         
parameter s5 = 2'b01;         
parameter s10 = 2'b10;        
parameter s15 = 2'b11;        

//Combinational logic
function [2:0] fsm;
	input [1:0] fsm_coin;         
	input [1:0] fsm_PRES_STATE;

	reg fsm_newspaper;                  
	reg [1:0] fsm_NEXT_STATE;

begin
	case (fsm_PRES_STATE)
	s0: //state = s0
	begin
		if (fsm_coin == 2'b10)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s10;
		end
		else if (fsm_coin == 2'b01)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s5;
		end
		else 
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s0;
		end
	end
	s5: //state = s5
	begin
		if (fsm_coin == 2'b10)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s15;
		end
		else if (fsm_coin == 2'b01)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s10;
		end
		else 
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s5;
		end
	end
	s10: //state = s10
	begin
		if (fsm_coin == 2'b10)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s15;
		end
		else if (fsm_coin == 2'b01)
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s15;
		end
		else 
		begin
			fsm_newspaper = 1'b0;
			fsm_NEXT_STATE = s10;
		end
	end
	s15: //state = s15
	begin
		fsm_newspaper = 1'b1;
		fsm_NEXT_STATE = s0;
	end
	endcase
	fsm = {fsm_newspaper, fsm_NEXT_STATE};
end
endfunction

//Reevaluate combinational logic each time a coin
//is put or the present state changes
assign {newspaper, NEXT_STATE} = fsm(coin, PRES_STATE);

//clock the state flipflops.  
//use synchronous reset
always @(posedge clock)
begin
	if (reset == 1'b1)
		PRES_STATE =  s0;
	else
		PRES_STATE =  NEXT_STATE;
end

endmodule
