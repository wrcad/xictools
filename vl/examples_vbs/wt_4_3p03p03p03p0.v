module wt_4_3p03p03p03p0(
                             RESULT,
                             NET_0_0,
                             NET_0_1,
                             NET_0_2,
                             NET_0_3
                          );

output [5:0] RESULT;

input  [3:0] NET_0_0;
input  [3:0] NET_0_1;
input  [3:0] NET_0_2;
input  [3:0] NET_0_3;

wire [5:0] RESULT;

wire   [3:0] net_0_0;
wire   [3:0] net_0_1;
wire   [3:0] net_0_2;
wire   [3:0] net_0_3;

assign net_0_0 = NET_0_0;
assign net_0_1 = NET_0_1;
assign net_0_2 = NET_0_2;
assign net_0_3 = NET_0_3;

//Sergio: Header generation ends here

wire [4:0] net_1_0;
wire [5:1] net_1_1;
wire [3:0] net_1_2;

wire [5:0] net_2_0;
wire [5:1] net_2_1;

wire [5:0] net_3_0;

//Sergio: Net generation ends here

initial
  begin
  $monitor(" net_2_0 %b\n",net_2_0);
  $monitor(" net_2_1 %b\n",net_2_1);
  $monitor(" net_1_0 %b\n",net_1_0);
  $monitor(" net_1_1 %b\n",net_1_1);
  $monitor(" net_1_2 %b\n",net_1_2);
 end

assign net_1_0[0] = net_0_0[0]^net_0_1[0]^net_0_2[0];
assign net_1_0[1] = net_0_0[1]^net_0_1[1]^net_0_2[1];
assign net_1_0[2] = net_0_0[2]^net_0_1[2]^net_0_2[2];
assign net_1_0[3] = net_0_0[3]^net_0_1[3]^net_0_2[3];
assign net_1_0[4] = net_0_0[3]^net_0_1[3]^net_0_2[3];

assign net_1_1[1] = (net_0_0[0] & net_0_1[0]) | ((net_0_0[0] ^ net_0_1[0]) & net_0_2[0]);
assign net_1_1[2] = (net_0_0[1] & net_0_1[1]) | ((net_0_0[1] ^ net_0_1[1]) & net_0_2[1]);
assign net_1_1[3] = (net_0_0[2] & net_0_1[2]) | ((net_0_0[2] ^ net_0_1[2]) & net_0_2[2]);
assign net_1_1[4] = (net_0_0[3] & net_0_1[3]) | ((net_0_0[3] ^ net_0_1[3]) & net_0_2[3]);
assign net_1_1[5] = (net_0_0[3] & net_0_1[3]) | ((net_0_0[3] ^ net_0_1[3]) & net_0_2[3]);

assign net_1_2 = net_0_3;

assign net_2_0[0] = net_1_0[0]^net_1_2[0];
assign net_2_0[1] = net_1_0[1]^net_1_1[1]^net_1_2[1];
assign net_2_0[2] = net_1_0[2]^net_1_1[2]^net_1_2[2];
assign net_2_0[3] = net_1_0[3]^net_1_1[3]^net_1_2[3];
assign net_2_0[4] = net_1_0[4]^net_1_1[4]^net_1_2[3];
assign net_2_0[5] = net_1_0[4]^net_1_1[5]^net_1_2[3];

assign net_2_1[1] = net_1_2[0] & net_1_0[0];
assign net_2_1[2] = (net_1_0[1] & net_1_1[1]) | ((net_1_0[1] ^ net_1_1[1]) & net_1_2[1]);
assign net_2_1[3] = (net_1_0[2] & net_1_1[2]) | ((net_1_0[2] ^ net_1_1[2]) & net_1_2[2]);
assign net_2_1[4] = (net_1_0[3] & net_1_1[3]) | ((net_1_0[3] ^ net_1_1[3]) & net_1_2[3]);
assign net_2_1[5] = (net_1_0[4] & net_1_1[4]) | ((net_1_0[4] ^ net_1_1[4]) & net_1_2[3]);


assign net_3_0 = { net_2_1, 1'b0 } + net_2_0;

assign RESULT = net_3_0;

endmodule //

