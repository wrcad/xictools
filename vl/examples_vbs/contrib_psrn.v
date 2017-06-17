/*

From: "Robb Cole" <n2vjo@amsat.org>
Subject: Beginner experience with Verilog
Date: 10 Jul 1997 19:14:05 GMT

A few weeks ago, Yoshikatsu Nakayama posted a request to this newsgroup
requesting some help with a piece of Verilog code he had written and was
having problems getting to work... I came up with some problem areas and
gave him a couple of ideas on how to fix it, which he implemented.  

A few days ago, I got the following note from Yoshikatsu, and asked his
permission to post it here for the edification of others who may just be
starting with Verilog.

------ posted with permission -----
 Hello,thank you for advice about Verilog code,the other day.
I tried,but it also doesn't work.Ah....(ToT)

 And I can't solve the problem,please help me again 
if you have time.I'm sorry for preventing your work.
	 
I'm using simulation tool named veriwell.

The fixed code is 
--------------------------------------------------------------------------------
*/

module psrn(start, init, rn);
	
	input start, init;
	output [15:0] rn;

	reg file [0:15];
	reg file2 [0:15];
	reg [15:0] rn;
	

	integer i;
	parameter [15:0] seed = 16'ha3;
	
	always @(start) begin
			
		if(init) 
		begin
			for(i=0; i<=15; i=i+1)
			begin	
				file2[i] = seed[i];
				file[i]  = seed[i];		
			end
		end	
		
		file2[15] = 1'b0 ^ file[15] ^ file[14];
			
		file2[14] = file[15] ^ file[14] ^ file[13];	
          
   	 	file2[13] = file[14]^file[12];
		file2[12] = file[11]^file[12]^file[13];
		file2[11] = file[10]^file[11]^file[12];
		file2[10] = file[9]^file[11];
		file2[9]  = file[8]^file[9]^file[10];
		file2[8]  = file[7]^file[8]^file[9];
		file2[7]  = file[6]^file[8];
		file2[6]  = file[5]^file[6]^file[7];
		file2[5]  = file[4]^file[5]^file[6];
		file2[4]  = file[3]^file[5];
		file2[3]  = file[2]^file[3]^file[4];
		file2[2]  = file[1]^file[2]^file[3];
		file2[1]  = file[0]^file[2];
		file2[0]  = file[0]^file[1]^1'b0; 
			
	assign rn = {file2[0], file2[1], file2[2], file2[3], file2[4], file2[5],
				 file2[6], file2[7], file2[8], file2[9], file2[10], file2[11],
				 file2[12], file2[13], file2[14], file2[15]};	
    end 		
	endmodule

module test;
	
  reg  clk, start, init;
  wire [15:0] rn;	
  parameter STEP = 100;
	
  psrn psrn(start, init, rn);
	
  always #(STEP) clk = ~clk;	
	
  initial
    begin	
      //$vw_dumpvars;		

      clk = 0; 
      start = 0;
      init = 0;
				
#STEP init = 1;
#STEP start = 1;
/*
      #STEP start = 1;
      #STEP init = 1;
*/
      #(STEP*5)
			
      $stop;
			
      $finish;
    end

  initial $monitor($stime, " clk = %b rn = %h ",clk ,rn );
endmodule		

/*
--------------------------------------------------------------------------------

Yoshikatsu's problem is that there is a sequencing dependency in the psrn
module.  Note that if the user sets the start flag before setting the init
flag, the variables file and file2 are still not initialized (and therefore
set to x).  So in the test module, at line 19, where Yoshikatsu sets the
start flag, the result of the XORing in the psrn module gives all x's in
the rn variable.  
Then, when he sets the init flag at line 20, he clears the file and file2
variables.  Had Yoshikatsu set the start flag to zero after line 20 and
before line 21, he would have seen the psrn module behave more like he had
expected.

The quick fix for Yoshikatsu was to swap the order of the set statements in
lines 19 and 20 so that the init command occurs first, which is the fix he
implemented.  I challenged Yoshikatsu to find a way to remove the
sequencing dependency described above.. He is working on it now, and will
report here what he came up with.

Again, this posting is just for the newbies to Verilog to give them
something to look at and see if they can figure it out.  I will post more
on this as Yoshikatsu solves his problems.

Now, for those still interested..  Is the psrn module above synthesizable? 
if not, how could you make it so?

Robb (trying his hand at net-teaching) Cole

*/
