//Define a module hello_world
module hello_world;
parameter id_num = 0; //define a module identification number = 0

initial //display the module identification number
        $display("Displaying hello_world id number = %d", id_num);
endmodule


//define top level module
module top;

//instantiate two hello_world modules; pass new parameter values
hello_world #(1) w1();
hello_world #(2) w2();


endmodule


