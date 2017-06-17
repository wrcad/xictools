module test;

   wire [10:0] a = 7'd 16;
   
   initial
     begin
        #1;
        $display(">%d<", a);
        $display(">%4d<", a);
        $display(">%h<", a);
        $display(">%4h<", a);
        $display("%d, %d", a);
     end
   
endmodule

