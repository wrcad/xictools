

module vector;
parameter maxsize = 100;

reg [4:0] op [0:maxsize];    // maxsize can be a parameter or a `define
reg [31:0] operand [0:maxsize];
reg [9:0] displacement [0:maxsize];

`define vectr1 {op[i], operand[i], displacement[i]} =
`define vector i=i+1;`vectr1

integer i;

initial
  begin
    i = 0;
//              op      operand     displacement
     `vectr1 {5'h12,  32'h12345678,   10'he74};
     `vector {5'h04,  32'h000054ea,   10'h0};
     `vector {5'h10,  32'h05008e19,   10'h52e};
     `vector {5'h1e,  32'h000054ea,   10'hda0};

//Also, techniques can be used that modify previous values:
     `vector {5'h1e,  operand[i-1] << 2, 10'h283}; // shift last operand by 2

     repeat(5)
       begin
         `vector {5'h1e, operand[i-1]+1, 10'h283}; // inc oper. by 1, 5 times
       end

// This sequence implements a walking 1: 1, 10, 100, 1000, etc.
          `vector {5'h1e, 32'h1,      10'habc};
     repeat(31)
       begin
         `vector {5'h1e, operand[i-1]<<1, displacement[i-1]};
       end

// This sequence implements a growing 1: 1, 11, 111, 1111, etc.
          `vector {5'h1e, 32'h1,      10'habc};
     repeat(31)
       begin
         `vector {5'h1e, (operand[i-1]+1<<1)-1, displacement[i-1]};
       end

  end
endmodule

module test;

vector vect();  // instantiate the vector module
integer iteration;
reg [4:0] op;
reg [31:0] oper;
reg [10:0] disp;
integer i;

initial
  for (i = 0; i < 100; i = i + 1)
    begin
      op = vect.op[i];
      oper = vect.operand[i];
      disp = vect.displacement[i];

      $display (op,,oper,,disp);

    end
endmodule
