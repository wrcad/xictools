module VNAND (out, in0, in1);
input in0;
input in1;
output out;

//timing information, rise/fall and min:typ:max
specify
(in0 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
(in1 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

nand (out, in0, in1);
endmodule

module VAND (out, in0, in1);
input in0;
input in1;
output out;

//timing information, rise/fall and min:typ:max
specify
(in0 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
(in1 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

and (out, in0, in1);
endmodule

module VNOR (out, in0, in1);
input in0;
input in1;
output out;

//timing information, rise/fall and min:typ:max
specify
(in0 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
(in1 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

nor (out, in0, in1);
endmodule

module VOR (out, in0, in1);
input in0;
input in1;
output out;

//timing information, rise/fall and min:typ:max
specify
(in0 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
(in1 => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

or (out, in0, in1);
endmodule

module VNOT (out, in);
input in;
output out;

//timing information, rise/fall and min:typ:max
specify
(in => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

not (out, in);
endmodule

module VBUF (out, in);
input in;
output out;

//timing information, rise/fall and min:typ:max
specify
(in => out) = (0.260604:0.513000:0.955206, 0.255524:0.503000:0.936586);
endspecify

buf (out, in);
endmodule

//D NEG-EDGE FLIP FLOP W/PRESET AND CLEAR
module NDFF
    (q,qbar,prebar,clrbar,d,clk);
    input
        prebar,clrbar,d,clk;
    output
        q,qbar;
    wire
        o1,o2,o3,o4,clk_;
    buf b1(q,qq);
    buf b2(qbar,qqbar);
    buf B3(clr,clrbar);
    buf B4(pre,prebar);
    buf B5(ck,clk);
    not
        n0(clk_,ck);
    nand
        n1(o1,pre,o2,o4),
        n2(o2,clr,o1,clk_),
        n3(o3,clk_,o4,o2),
        n4(o4,clr,d,o3);
    nand
        n5(qq,pre,qqbar,o2),
        n6(qqbar,clr,qq,o3);
endmodule

//D POS-EDGE FLIP FLOP W/PRESET AND CLEAR
module PDFF
    (q,qbar,prebar,clrbar,d,clk);
    input
        prebar,clrbar,d,clk;
    output
        q,qbar;
    wire
        o1,o2,o3,o4;
    buf b1(q,qq);
    buf b2(qbar,qqbar);
    buf B3(clr,clrbar);
    buf B4(pre,prebar);
    buf B5(ck,clk);
    nand
        n1(o1,pre,o2,o4),
        n2(o2,clr,o1,ck),
        n3(o3,ck,o4,o2),
        n4(o4,clr,d,o3);
    nand
        n5(qq,pre,qqbar,o2),
        n6(qqbar,clr,qq,o3);
endmodule
 


/*
module QT_DFF_ECK(D,CK,CE,Q);
    input D, CK, CE;
    output Q;
    wire in_data;
    wire CE_NOT;
    wire CE_NOT_AND_Q;
    wire CE_AND_D;
 
    not (CE_NOT, CE);
    and (CE_NOT_AND_Q, CE_NOT, Q);
    and (CE_AND_D, CE, D);
    or (in_data, CE_NOT_AND_Q, CE_AND_D);
    PDFF a0(Q,,1'b1,1'b1,in_data,CK);
endmodule

module QT_DFF_R_ECK(D,CK,CE,R,Q);
    input D, CK, CE, R;
    output Q;
    wire in_data;
    wire CE_NOT;
    wire CE_NOT_AND_Q;
    wire CE_AND_D;
    wire R_NOT;
 
    not (CE_NOT, CE);
    and (CE_NOT_AND_Q, CE_NOT, Q);
    and (CE_AND_D, CE, D);
    or (in_data, CE_NOT_AND_Q, CE_AND_D);
    not (R_NOT, R);
    PDFF a0(Q,,1'b1,R_NOT,in_data,CK);
endmodule
*/


/*
module VBUFIF1 (out, in, control);
output out;
input in;
input control;

bufif1 (out, in, control);
endmodule
*/
