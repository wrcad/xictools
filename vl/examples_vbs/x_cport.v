module main;
    reg a, b ;
    wire x, y ;

    mod2 mod1_inst1( x, 1'b0 );
    mod2 mod1_inst2( y, b );

    initial begin
	$monitor("%d: x,y = %b,%b\n", $time, x, y );
	a = 0 ;
	b = 1 ;
	#1 ;
	a = 1 ;
	b = 0 ;

	#2 $write("Expected output:\n");
	$write("0: x,y = 0,1\n");
	$write("1: x,y = 0,0\n");

    end
endmodule


module mod1( z, d );
    output z ;
    input d ;

    wire z ;
    mod2 mod2_instance( z, d );
endmodule


module mod2( q, e );
    output q ;
    input e ;

    wire q ;
    assign q = e ;
endmodule
