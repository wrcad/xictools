module main;

wire        [5:0] result;

reg         [5:0] res;

wire [3:0] net_0_0;
wire [3:0] net_0_1;
wire [3:0] net_0_2;
wire [3:0] net_0_3;

reg         clk;

// Sign extend
// Extends to 32 bit 2's complement number
//
function     [31:0] se;
  input      [31:0] x;
  input      [31:0] signpos;
  reg        [31:0] res;
  reg        [31:0] i;
  begin
    for(i=0;i<signpos;i=i+1)
      begin
	res[i] = x[i];
      end
    for(i=signpos;i<32;i=i+1)
      begin
        res[i] = x[signpos];
      end
    se = res;
  end
endfunction // se


wt_4_3p03p03p03p0 u1(
result,
net_0_0,
net_0_1,
net_0_2,
net_0_3
);

always @(clk)
begin
    clk = #5 !clk;
end

always@(posedge clk)
begin
   net_0_0 <= $random;
   net_0_1 <= $random;
   net_0_2 <= $random;
   net_0_3 <= $random;
end

initial
begin
              clk=0;
    #100000 $finish;
end

always@(posedge clk)
begin
  res = 0
        + se(net_0_0,3)
        + se(net_0_1,3)
        + se(net_0_2,3)
        + se(net_0_3,3);
if(res != result)
    $write(
      "%d %b %b %b %b %b %b error\n",
      $time,
      result,
      res,
      net_0_0,
      net_0_1,
      net_0_2,
      net_0_3
    );
else
    $write(
      "%d %b %b %b %b %b %b\n",
      $time,
      result,
      res,
      net_0_0,
      net_0_1,
      net_0_2,
      net_0_3
    );
end

endmodule // wt_4_3p03p03p03p0_test
