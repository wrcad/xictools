module main;
    reg a, b ;
    wire x, y ;

    mod1 mod1_inst1( x, a );
    mod1 mod1_inst2( y, b );

    initial begin
	$monitor("%d: x,y = %b,%b\n", $time, x, y );
	a = 0 ;
	b = 1 ;
	#1 ;
	a = 1 ;
	b = 0 ;

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
