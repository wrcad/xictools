/*
From: jayl@latticesemi.com (Jay Lessert)
Subject: Re: help: verilog hangs during compilation
Date: 21 Oct 1997 14:38:21 -0700

In article <62isi9$q4n$1@tribune.usask.ca>,
Henry F Fernandes <hff135@skorpio3.usask.ca> wrote:
>the problem is this: verilog freezes during the compilationi/linking process of 
>a project i am working on.
>
>initially, i implemented a prototype for this project. verilog handled the
>prototype with no problems. it compiled in almost no time (0.1 sec).
>
>the only difference between the prototype and the final version is that the
>final version uses much larger memory blocks. specifically, there are two
>seperate memories of size 2^16 bits x 14 bits, and one of size 2^19 x 8 bits.

You haven't shown us exactly what you're doing, or what OS/HW you're
running, so it's hard to tell.  Assuming you're declaring something
like:

    reg [13:0] mem0  [65535:0];
    reg  [7:0] mem1 [524287:0];

your memories should only take up a few MB of process size.  I just did
a trivial test case in Verilog-XL 2.5:
*/

module memsize();
    reg [13:0] mem0  [65535:0];
    reg  [7:0] mem1 [524287:0];
    integer i;
    initial begin
	$display("Hello, world!");
	for (i=0; i<65536; i=i+1)
	    begin
	    mem0[i] = 14'b10101010101010;
	    end
	for (i=0; i<524288; i=i+1)
	    begin
	    mem1[i] = 8'b10101010;
	    end
	$stop(2);
    end
endmodule

/*
and get a total process size of 5MB (SunOS 4.1.4, SS10/60).  I used
2.3.3 for quite a while and don't expect it to behave significantly
differently:

USER       PID %CPU %MEM   SZ  RSS TT STAT START  TIME COMMAND
jayl     12706 23.8  7.1 4332 5372 p6 T    14:33   0:03 verilog -s mem.v

    Hello, world!
    L15 "mem.v": $stop at simulation time 0
    Data structure takes 1393932 bytes of memory
    1179654 simulation events
    CPU time: 0.3 secs to compile + 0.5 secs to link + 3.3 secs in simulation
    C1 > .
    1179654 simulation events
    CPU time: 0.3 secs to compile + 0.5 secs to link + 3.3 secs in simulation
    End of VERILOG-XL 2.5   Oct 21, 1997  14:20:42

Of course, you may have made some other mistake.  I suppose it is also
possible that you're right on the edge of running out of physical
memory, the extra memory pushes you over the line, and you have
mistaken massive paging for freezing.  The appropriate system monitoring
tool (vmstat on SunOS) will tell you that.
-- 
Jay Lessert                              jay_lessert@latticesemi.com
Lattice Semiconductor Corp.                    (voice)1.503.681.0118
Hillsboro, OR, USA                               (fax)1.503.693.0540
*/
