/*
From coconut!news.pixi.com!news.zeitgeist.net!cygnus.com!kithrup.com!news.Stanford.EDU!agate!howland.erols.net!cam-news-hub1.bbnplanet.com!news.mathworks.com!newsfeed.internetmci.com!EU.net!usenet2.news.uk.psi.net!uknet!usenet1.news.uk.psi.net!uknet!uknet!newsfeed.ed.ac.uk!news Tue Sep 17 21:34:11 1996
Path: coconut!news.pixi.com!news.zeitgeist.net!cygnus.com!kithrup.com!news.Stanford.EDU!agate!howland.erols.net!cam-news-hub1.bbnplanet.com!news.mathworks.com!newsfeed.internetmci.com!EU.net!usenet2.news.uk.psi.net!uknet!usenet1.news.uk.psi.net!uknet!uknet!newsfeed.ed.ac.uk!news
From: Gerard M Blair <gerard@ee.ed.ac.uk>
Newsgroups: comp.lang.verilog
Subject: non-blocking statements
Date: Tue, 17 Sep 1996 14:57:20 +0100
Organization: University of Edinburgh, Scotland, UK
Lines: 131
Message-ID: <323EAE40.6377@ee.ed.ac.uk>
NNTP-Posting-Host: sakura.ee.ed.ac.uk
Mime-Version: 1.0
Content-Type: multipart/mixed; boundary="------------62BC7B753A98"
X-Mailer: Mozilla 3.0Gold (X11; I; SunOS 5.5.1 sun4m)

This is a multi-part message in MIME format.

--------------62BC7B753A98
Content-Type: text/plain; charset=us-ascii
Content-Transfer-Encoding: 7bit

I have a question about non-blocking assignments.

One of the well known properties on the non-blocking assignment
is that it allows a "swap" of two registers, thus:

a <= b;
b <= a;

For a class, I wanted to write a very simple median filter
model - so that they could implement a smarter version
based upon a published paper.

My bubble sort was coded thus:

        // sort workSpace
        for (i = 0; i < 5; i = i + 1)
        for (j = 5; j > i; j = j - 1)
            if (workSpace [j] < workSpace [j-1]) begin
                  workSpace [j-1] <= workSpace [j];
                  workSpace [j]   <= workSpace [j-1];
                  end

But - if fails to do much at all ! Of course it can
be done with a tmp register. 

Clearly, the above code would run into synthesis problems since
every register is potentially swapping values many times - and
this code only works as a behavioural model. however, I am curious
to know if it "should" work (in terms of the Standard) and if
not, how could I make it work.

However, if I add a delay of #1; to the end of the inner loop,
all is well (except fro the added delay).

I was thus surprised that #0; did not work as well.

In case anyone wants to play, the (buggy) code is below.

-- 
Gerard M Blair, Senior Lecturer, The Department of Electrical Engineering,
              The University of Edinburgh, Scotland, UK
Email: gerard@ee.ed.ac.uk   -  Home page: http://www.ee.ed.ac.uk/~gerard/

--------------62BC7B753A98
Content-Type: text/plain; charset=us-ascii; name="median.v"
Content-Transfer-Encoding: 7bit
Content-Disposition: inline; filename="median.v"
*/

// behavioural model of running 5-word median filter
module median_beh (outWord, clk, reset, inWord);
output [3:0] outWord;
input        clk, reset;
input  [3:0] inWord;

reg    [3:0] outWord;

integer count, i, j;
reg    [3:0] mem [0:4];
reg    [3:0] workSpace [0:4];
reg    [3:0] tmp;

initial begin
	count = 0;
	end

always @(posedge clk) begin
	if (reset == 0)
		for (i = 0; i < 5; i = i + 1)
			mem [i] = 0;
	else begin
		// update array of lst 5 words
		mem [count] = inWord;
		count = (count == 4) ? 0 : count + 1;

		// copy array into work space
		for (i = 0; i < 5; i = i + 1)
			workSpace [i] = mem [i];

		// sort workSpace
		for (i = 0; i < 5; i = i + 1)
//		for (j = 5; j > i; j = j - 1)
		for (j = 4; j > i; j = j - 1)
			if (workSpace [j] < workSpace [j-1]) begin
				workSpace [j-1] <= workSpace [j]; 
				workSpace [j]   <= workSpace [j-1]; 
				end
		
		// test output
		for (i = 0; i < 5; i = i + 1)
			$display ("     %d %d %d", i, mem[i], workSpace[i]);

		// output result
		outWord = workSpace[2];
		
		end
	end

endmodule

module testStimulus;

reg    [3:0] inWord;
reg          clk, reset;
wire   [3:0] outWord;


// instantiate behavioural model
median_beh beh (outWord, clk, reset, inWord);

initial begin
	clk = 0;
	#5003 $finish;
	end 

always #50 clk = ~clk;

always @(negedge clk)
	inWord = {$random} / 4 % 16;

always @(posedge clk)
	#2 $display("input word %d - ", inWord, "output word %d", outWord);

endmodule
/*
--------------62BC7B753A98--

*/
