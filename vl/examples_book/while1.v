module count;

//Illustration 1: Increment count from 0 to 127. 
//Exit at count 128. Display the count variable.
integer count;

initial
begin
        count = 0;

        while (count < 128) //Execute loop till count is 127. 
											//exit at count 128
        begin
            $display("Count = %d", count); 
						count = count + 1;
        end
end

endmodule

