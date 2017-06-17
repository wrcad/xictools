module counter;
//Illustration 1 : increment and display count from 0 to 127
integer count;

initial
begin
   count = 0;
   repeat(128)
   begin
        $display("Count = %d", count);
        count = count + 1;
   end
end

endmodule

