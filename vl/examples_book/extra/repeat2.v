//Illustration 2 : Data buffer module example
//After it receives a data_start signal. 
//Reads data for next 8 cycles.

module data_buffer(data_start, data, clock);

parameter cycles = 8;
input data_start;
input [15:0] data;
input clock;

reg [15:0] buffer [0:7];
integer i;

always @(posedge clock)
begin
  if(data_start) //data start signal is true
  begin
    i = 0;
    repeat(cycles) //Store data at the posedge of next 8 clock
                   //cycles
    begin
      @(posedge clock) buffer[i] = data; //waits till next
                                      	 // posedge to latch data
      i = i + 1;
    end
  end
end

endmodule
