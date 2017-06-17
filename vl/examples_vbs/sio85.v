/******************************************************************
 * NOTICE: The information contained in this file is proprietary  *
 * to Gateway Design Automation Corp. and is being made available *
 * to Gateway's customers under strict non-disclosure agreements. *
 * Use or disclosure of this information is permissible only      *
 * under the terms of the existing non-disclosure agreement.      *
 ******************************************************************/

/****************************************************************
 *                   SIO85 DEMONSTRATION                        *
 *  Simulation demonstration of the Intel 8085a microprocessor  *
 *  with 8080 program in RAM, and two 8251's communicating      *
 *  across a serial line                                        *
 ****************************************************************/

module s85; // simulation control module

    reg [8:1]      dflags;
    initial        dflags = 0;
    // diag flags:
    // 1 = printmem
    // 2 = dump state at end
    // 3 = test reset control
    // 4 = monitor the transmit and receive lines

    wire            s0, ale, rxd, txd;

    tri[7:0]        ad, a;
    tri1            read, write, iomout;

    reg             clock, trap, rst7p5, rst6p5, rst5p5,
                    intr, ready, nreset, hold, pclock;

    //instantiate the RAM module
    ram85a          r0(ale, ad, a, write, read, iomout);

    //instantiate the 8085a processor module
    intel_8085a     i85(clock, , , , , trap,
                        rst7p5, rst6p5, rst5p5, intr, ,
                        ad, a, s0, ale, write, read, ,
                        iomout, ready, nreset,
                        , , hold);

    //instantiate the 8251 peripheral interchange module
    i8251           p1(ad, rxd, , pclock, write, !iomout,
                        a[0], read, ,
                        , , , , txd,
                        , !nreset, , , , pclock, );
    defparam p1.instance_id = 8'h01;
        
    //instantiate the 8251 test module
    test8251        t1(txd, rxd);

    initial
        begin
            if(dflags[4])
                $monitor($time,,txd,,rxd);
            clock = 0;
            pclock = 0;
            nreset = 0;
            trap = 0;
            rst7p5 = 0;
            rst6p5 = 0;
            rst5p5 = 0;
            intr = 0;
            ready = 0;
            hold = 0;

            #500
                begin
                    nreset = 1;
                    ready = 1;
                end

            @(posedge i85.haltff) @i85.ec2;
                disable clockwave;
                disable pclockwave;
                disable i85.run_processor;

            if(dflags[1]) r0.printmem;
            if(dflags[2]) i85.dumpstate;
        end

    initial
        begin :clockwave
            forever
                begin
                    #150 clock = 1; //->i85.ec1;
                    #150 clock = 0; //->i85.ec2;
                end
        end

    initial #100
        begin :pclockwave
            forever
                begin
                    #pc.pchperiod pclock = 1;
                    #pc.pchperiod pclock = 0;
                end
        end

    /* test reset control */
    initial if(dflags[3])
        begin
            # 5000 nreset = 0; #500 nreset = 1;
            # 6000 nreset = 0; #500 nreset = 1;
            #10000 nreset = 0; #500 nreset = 1;
            #20000 nreset = 0; #1000 nreset = 1;
        end

endmodule // s85



/* ram module for 8085a */
module ram85a(ale, ad, a, write, read, iomout);

    reg [8:1]      dflags;
    initial         dflags = 'b00000000;
    // diag flags:
    // 1 = trace read and writes
    // 2 = dump ram after loading

    inout[7:0] ad;
    input[7:0] a;
    input ale, write, read, iomout;

    reg[7:0] ad_reg;
    tri[7:0] ad = (read || iomout) ? 'bz : ad_reg;

    parameter ramsize = 'h7A;
    reg[7:0] ram[ramsize-1:0];

    reg[15:0] address;

    always @(negedge ale) begin
        address = {a, ad};
        if(dflags[1])
            $display("Reading %h into ram address", address);
    end
    always @(negedge read) begin
        if(dflags[1])
            $display("Reading %h = ram[%h]", ram[address], address);
        ad_reg = ram[address];
    end
    always @(negedge write) begin
        if(dflags[1])
            $display("Writing ram[%h] = %h", address, ad);
        ram[address] = ad;
    end

    //define contents of RAM
    initial begin
        ram['h0]='h31; ram['h1]='h7A; ram['h2]='h00; ram['h3]='h3E;
        ram['h4]='h4D; ram['h5]='hD3; ram['h6]='hF1; ram['h7]='h3E;
        ram['h8]='h05; ram['h9]='hD3; ram['ha]='hF1; ram['hb]='h21;
        ram['hc]='h4A; ram['hd]='h00; ram['he]='hCD; ram['hf]='h18;
        ram['h10]='h00; ram['h11]='hCD; ram['h12]='h23; ram['h13]='h00;
        ram['h14]='h23; ram['h15]='hC3; ram['h16]='h0E; ram['h17]='h00;
        ram['h18]='hDB; ram['h19]='hF1; ram['h1a]='hE6; ram['h1b]='h02;
        ram['h1c]='hCA; ram['h1d]='h18; ram['h1e]='h00; ram['h1f]='hDB;
        ram['h20]='hF0; ram['h21]='h77; ram['h22]='hC9; ram['h23]='h7E;
        ram['h24]='h0F; ram['h25]='h0F; ram['h26]='h0F; ram['h27]='h0F;
        ram['h28]='hCD; ram['h29]='h30; ram['h2a]='h00; ram['h2b]='h7E;
        ram['h2c]='hCD; ram['h2d]='h30; ram['h2e]='h00; ram['h2f]='hC9;
        ram['h30]='hE6; ram['h31]='h0F; ram['h32]='hFE; ram['h33]='h0A;
        ram['h34]='hD2; ram['h35]='h3C; ram['h36]='h00; ram['h37]='hC6;
        ram['h38]='h30; ram['h39]='hC3; ram['h3a]='h3E; ram['h3b]='h00;
        ram['h3c]='hC6; ram['h3d]='h37; ram['h3e]='h47; ram['h3f]='hDB;
        ram['h40]='hF1; ram['h41]='hE6; ram['h42]='h01; ram['h43]='hCA;
        ram['h44]='h3E; ram['h45]='h00; ram['h46]='h78; ram['h47]='hD3;
        ram['h48]='hF0; ram['h49]='hC9;

        /* last address of this program is 007AH (STACK) */

        if(dflags[2])
            printmem;
    end

    //print out the contents of the RAM
    task printmem;
    integer i;
    reg[7:0] data;
    begin
        $write("dump of ram");
        for(i = 0; i < ramsize; i = i + 1)
        begin
            data = ram[i];
            if((i % 4) == 0) $write(" ");
            if((i % 16) == 0) $write("\n%h: ", i);
            $write("%h", data);
            $write(" ");
        end
        $write("\n\n");
    end
    endtask
endmodule




/* Behavioral description of the Intel 8085a microprocessor */
module intel_8085a
        (clock, x2, resetff, sodff, sid, trap,
         rst7p5, rst6p5, rst5p5, intr, intaff,
         ad, a, s0, aleff, writeout, readout, s1,
         iomout, ready, nreset,
         clockff, hldaff, hold);

    reg [8:1]      dflags;
    initial         dflags = 'b011;
    // diag flags:
    // 1 = trace instructions
    // 2 = trace IN and OUT instructions
    // 3 = trace instruction count

    output
        resetff, sodff, intaff, s0, aleff,
        writeout, readout, s1, iomout, clockff, hldaff;

    inout[7:0] ad, a;

    input
            clock, x2, sid, trap,
            rst7p5, rst6p5, rst5p5,
            intr, ready, nreset, hold;

    reg[15:0]
        pc,        // program counter
        sp,        // stack pointer
        addr;      // address output

    reg[8:0]
        intmask;   // interrupt mask and status

    reg[7:0]
        acc,       // accumulator
        regb,      // general
        regc,      // general
        regd,      // general
        rege,      // general
        regh,      // general
        regl,      // general
        ir,        // instruction
        data;      // data output

    reg
        aleff,     // address latch enable
        s0ff,      // status line 0
        s1ff,      // status line 1
        hldaff,    // hold acknowledge
        holdff,    // internal hold
        intaff,    // interrupt acknowledge
        trapff,    // trap interrupt request
        trapi,     // trap execution for RIM instruction
        inte,      // previous state of interrupt enable flag
        int,       // interrupt acknowledge in progress
        validint,  // interrupt pending
        haltff,    // halt request
        resetff,   // reset output
        clockff,   // clock output
        sodff,     // serial output data
        read,      // read request signal
        write,     // write request signal
        iomff,     // i/o memory select
        acontrol,  // address output control
        dcontrol,  // data output control
        s,         // data source control
        cs,        // sign condition code
        cz,        // zero condition code
        cac,       // aux carry condition code
        cp,        // parity condition code
        cc;        // carry condition code

    wire
        s0 = s0ff & ~haltff,
        s1 = s1ff & ~haltff;

    tri[7:0]
        ad = dcontrol ? (s ? data : addr[7:0]) : 'bz,
        a = acontrol ? addr[15:8] : 'bz;

    tri
        readout = acontrol ? read : 'bz,
        writeout = acontrol ? write : 'bz,
        iomout = acontrol ? iomff : 'bz;

    event
        ec1, // clock 1 event
        ec2; // clock 2 event

    // internal clock generation
    always begin
        @(posedge clock) -> ec1;
        @(posedge clock) -> ec2;
    end

    integer instruction; // instruction count
    initial instruction = 0;

    always begin:run_processor
        #1 reset_sequence;
        fork                                            
            execute_instructions;             // Instructions executed
            wait(!nreset)                     // in parallel with reset  
                @ec2 disable run_processor; // control. Reset will     
        join                                  // disable run_processor   
    end                                       // and all tasks and       
                                              // functions enabled from  
                                              // it when nreset set to 0.

    task reset_sequence;
    begin
        wait(!nreset)
        fork
            begin
                $display("Performing 8085(%m) reset sequence");
                read = 1;
                write = 1;
                resetff = 1;
                dcontrol = 0;
                @ec1 // synchronized with clock 1 event
                    pc = 0;
                    ir = 0;
                    intmask[3:0] = 7;
                    intaff = 1;
                    acontrol = 0;
                    aleff = 0;
                    intmask[7:5] = 0;
                    sodff = 0;
                    trapff = 0;
                    trapi = 0;
                    iomff = 0;
                    haltff = 0;
                    holdff = 0;
                    hldaff = 0;
                    validint = 0;
                    int = 0;
                disable check_reset;
            end
            begin:check_reset
                wait(nreset)               // Check, in parallel with the
                    disable run_processor; // reset sequence, that nreset
            end                            // remains at 0.
        join
        wait(nreset) @ec1 @ec2 resetff = 0;
    end
    endtask


    /* fetch and execute instructions */
    task execute_instructions;
    forever begin
        instruction = instruction + 1;
        if(dflags[3])
            $display("executing instruction %d", instruction);

        @ec1 // clock cycle 1
            addr = pc;
            s = 0;
            iomff = 0;
            read = 1;
            write = 1;
            acontrol = 1;
            dcontrol = 1;
            aleff = 1;
            if(haltff) begin
                haltff = 1;
                s0ff = 0;
                s1ff = 0;
                haltreq;
            end
            else begin
                s0ff = 1;
                s1ff = 1;
            end
        @ec2
            aleff = 0;

        @ec1 // clock cycle 2
            read = 0;
            dcontrol = 0;
        @ec2
            ready_hold;

        @ec2 // clock cycle 3
            read = 1;
            data = ad;
            ir = ad;

        @ec1 // clock cycle 4
            if(do6cycles(ir)) begin
                // do a 6-cycle instruction fetch
                @ec1 @ec2 // conditional clock cycle 5
                    if(hold) begin
                        holdff =1 ;
                        acontrol = 0;
                        dcontrol = 0;
                        @ec2 hldaff = 1;
                    end
                    else begin
                        holdff = 0;
                        hldaff = 0;
                    end

                @ec1; // conditional clock cycle 6
            end

            if(holdff) holdit;
            checkint;
            do_instruction;

            while(hold) @ec2 begin
                acontrol = 0;
                dcontrol = 0;
            end
            holdff = 0;
            hldaff = 0;
            if(validint) interrupt;
    end
    endtask


    function do6cycles;
    input[7:0] ireg;
    begin
        do6cycles = 0;
        case(ireg[2:0])
            0, 4, 5, 7: if(ireg[7:6] == 3) do6cycles = 1;
            1: if((ireg[3] == 1) && (ireg[7:5] == 7)) do6cycles = 1;
            3: if(ireg[7:6] == 0) do6cycles = 1;
        endcase
    end
    endfunction


    task checkint;
    begin
        if(rst6p5)
            if((intmask[3] == 1) && (intmask[1] == 0)) intmask[6] = 1;
        else
            intmask[6] = 0;

        if(rst5p5)
            if((intmask[3] == 1) && (intmask[0] == 0)) intmask[5] = 1;
        else
            intmask[5] = 0;

        if({intmask[7], intmask[3:2]} == 6)
            intmask[4] = 1;
        else
            intmask[4] = 0;

        validint = (intmask[6:4] == 7) | trapff | intr;
    end
    endtask


    // concurently with executing instructions,
    // process primary inputs for processor interrupt
    always @(posedge trap) trapff = 1;

    always @(negedge trap) trapff = 0;

    always @(posedge rst7p5) intmask[7] = 1;


    /* check condition of ready and hold inputs */
    task ready_hold;
    begin
        while(!ready) @ec2;
        @ec1
            if(hold) begin
                holdff = 1;
                @ec2 hldaff = 1;
            end
    end
    endtask


    /* hold */
    task holdit;
    begin
        while(hold) @ec2 begin
            acontrol = 0;
            dcontrol = 0;
        end
        holdff = 0;
        @ec2 hldaff = 0;
    end
    endtask



    /* halt request */
    task haltreq;
    forever begin
        @ec2
            if(validint) begin
                haltff = 0;
                interrupt;
                disable haltreq;
            end
            else begin
                while(hold) @ec2 hldaff = 1;
                hldaff = 0;
                @ec2;
            end

        @ec1 #10
            dcontrol = 0;
            acontrol = 0;
            checkint;
    end
    endtask



    /* memory read */
    task memread;
    output[7:0] rdata;
    input[15:0] raddr;
    begin
        @ec1
            addr = raddr;
            s = 0;
            acontrol = 1;
            dcontrol = 1;
            iomff = int;
            s0ff = int;
            s1ff = 1;
            aleff = 1;
        @ec2
            aleff = 0;

        @ec1 
            dcontrol = 0;
            if(int)
                intaff = 0;
            else
                read = 0;
        @ec2
            ready_hold;
            checkint;

        @ec2
            intaff = 1;
            read = 1;
            rdata = ad;
        if(holdff) holdit;
    end
    endtask



    /* memory write */
    task memwrite;
    input[7:0] wdata;
    input[15:0] waddr;
    begin
        @ec1
            aleff = 1;
            s0ff = 1;
            s1ff = 0;
            s = 0;
            iomff = 0;
            addr = waddr;
            acontrol = 1;
            dcontrol = 1;
        @ec2
            aleff = 0;

        @ec1
            data = wdata;
            write = 0;
            s = 1;
        @ec2
            ready_hold;
            checkint;

        @ec2
            write = 1;
        if(holdff) holdit;
    end
    endtask

    /* reads from an i/o port */
    task ioread;
    input[7:0] sa;
    begin
        @ec1
            aleff = 1;
            s0ff = 0;
            s1ff = 1;
            s = 0;
            iomff = 1;
            addr = {sa, sa};
            acontrol = 1;
            dcontrol = 1;

        @ec2
            aleff = 0;

        @ec1
            dcontrol = 0;
            if(int)
                intaff = 0;
            else
                read = 0;

        @ec2
            ready_hold;

            checkint;

        @ec2
            intaff = 1;
            read = 1;
            acc = ad;
            if(dflags[2])
                $display("IN %h   data = %h", sa, acc);
    end
    endtask


    /* writes into i/o port */
    task iowrite;
    input[7:0] sa;
    begin
        @ec1
            addr = {sa, sa};
            aleff = 1;
            s0ff = 1;
            s1ff = 0;
            s = 0;
            iomff = 1;
            acontrol = 1;
            dcontrol = 1;

        @ec2
            aleff = 0;

        @ec1
            data = acc;
            write = 0;
            s = 1;

            if(dflags[2])
                $display("OUT %h   data = %h", sa, acc);

        @ec2
            ready_hold;

            checkint;

        @ec2
            write = 1;
            if(holdff) holdit;
    end
    endtask


    task interrupt;
    begin
        @ec1
            if(hold) begin
                holdff = 1;
                holdit;
                @ec2 hldaff = 1;
            end
            if(trapff) begin
                inte = intmask[3];
                trapi = 1;
                intic;
                pc = 'h24;
                trapi = 1;
                trapff = 0;
            end
            else if(intmask[7]) begin
                intic;
                pc = 'h3c;
                intmask[7] = 0;
            end
            else if(intmask[6]) begin
                intic;
                pc = 'h34;
                intmask[6] = 0;
            end
            else if(intmask[5]) begin
                intic;
                pc = 'h2c;
                intmask[5] = 0;
            end
            else if(intr) begin
                //?
            end
    end
    endtask

    task intic;
    begin
        aleff = 1;
        s0ff = 1;
        s1ff = 1;
        s = 0;
        iomff = 1;
        addr = pc;
        read = 1;
        write = 1;
        acontrol = 1;
        dcontrol = 1;

        @ec2 aleff = 0;
        @ec1 dcontrol = 0;
        repeat(4) @ec1;
        push2b(pc[15:8], pc[7:0]);
    end
    endtask




    /* execute instruction */
    task do_instruction;
    begin
        if(dflags[1])
            $display(
                "C%bZ%bM%bE%bI%b A=%h B=%h%h D=%h%h H=%h%h S=%h P=%h IR=%h",
                cc, cz, cs, cp, cac,
                acc, regb,regc, regd,rege, regh,regl,
                sp, pc, ir);

        pc = pc + 1;
        @ec2 // instruction decode synchronized with clock 2 event
            case(ir[7:6])
                0:
                    case(ir[2:0])
                        0: newops;
                        1: if(ir[3]) addhl; else lrpi;
                        2: sta_lda;
                        3: inx_dcx;
                        4: inr;
                        5: dcr;
                        6: movi;
                        7: racc_spec;
                    endcase
                1:
                    move;
                2:
                    rmop;
                3:
                    case(ir[2:0])
                        0,
                        2,
                        4: condjcr;
                        1: if(ir[3]) decode1; else pop;
                        3: decode2;
                        5: if(ir[3]) decode3; else push;
                        6: immacc;
                        7: restart;
                    endcase
            endcase
    end
    endtask




    /* move register to register */
    task move;
        case(ir[2:0])
            0: rmov(regb); // MOV -,B
            1: rmov(regc); // MOV -,C
            2: rmov(regd); // MOV -,D
            3: rmov(rege); // MOV -,E
            4: rmov(regh); // MOV -,H
            5: rmov(regl); // MOV -,L
            6:
                if(ir[5:3] == 6) haltff = 1; // HLT
                else begin // MOV -,M
                    memread(data, {regh, regl});
                    rmov(data);
                end

            7: rmov(acc); // MOV -,A
        endcase
    endtask

    /* enabled only by move */
    task rmov;
    input[7:0] fromreg;
        case(ir[5:3])
            0: regb = fromreg; // MOV B,-
            1: regc = fromreg; // MOV C,-
            2: regd = fromreg; // MOV D,-
            3: rege = fromreg; // MOV E,-
            4: regh = fromreg; // MOV H,-
            5: regl = fromreg; // MOV L,-
            6: memwrite(fromreg, {regh, regl}); // MOV M,-
            7: acc = fromreg; // MOV A,-
        endcase
    endtask



    /* move register and memory immediate */
    task movi;
    begin
        case(ir[5:3])
            0: memread(regb, pc); // MVI B
            1: memread(regc, pc); // MVI C
            2: memread(regd, pc); // MVI D
            3: memread(rege, pc); // MVI E
            4: memread(regh, pc); // MVI H
            5: memread(regl, pc); // MVI L
            6: // MVI M
                begin
                    memread(data, pc);
                    memwrite(data, {regh, regl});
                end

            7: memread(acc, pc); // MVI A
        endcase
        pc = pc + 1;
    end
    endtask





    /* increment register and memory contents */
    task inr;
        case(ir[5:3])
            0: doinc(regb); // INR B
            1: doinc(regc); // INR C
            2: doinc(regd); // INR D
            3: doinc(rege); // INR E
            4: doinc(regh); // INR H
            5: doinc(regl); // INR L
            6: // INR M
                begin
                    memread(data, {regh, regl});
                    doinc(data);
                    memwrite(data, {regh, regl});
                end

            7: doinc(acc); // INR A
        endcase
    endtask

    /* enabled only from incrm */
    task doinc;
    inout[7:0] sr;
    begin
        cac = sr[3:0] == 'b1111;
        sr = sr + 1;
        calpsz(sr);
    end
    endtask



    /* decrement register and memory contents */
    task dcr;
        case(ir[5:3])
            0: dodec(regb); // DCR B
            1: dodec(regc); // DCR C
            2: dodec(regd); // DCR D
            3: dodec(rege); // DCR E
            4: dodec(regh); // DCR H
            5: dodec(regl); // DCR L
            6: // DCR M
                begin
                    memread(data, {regh, regl});
                    dodec(data);
                    memwrite(data, {regh, regl});
                end

            7: dodec(acc); // DCR A
        endcase
    endtask

    /* enabled only from decrm */
    task dodec;
    inout[7:0] sr;
    begin
        cac = sr[3:0] == 0;
        sr = sr - 1;
        calpsz(sr);
    end
    endtask





    /* register and memory acc instructions */
    task rmop;
        case(ir[2:0])
            0: doacci(regb);
            1: doacci(regc);
            2: doacci(regd);
            3: doacci(rege);
            4: doacci(regh);
            5: doacci(regl);
            6:
                begin
                    memread(data, {regh, regl});
                    doacci(data);
                end

            7: doacci(acc);
        endcase
    endtask


    /* immediate acc instructions */
    task immacc;
    begin
        memread(data, pc);
        pc = pc + 1;
        doacci(data);
    end
    endtask


    /* operate on accumulator */
    task doacci;
    input[7:0] sr;
    reg[3:0] null4;
    reg[7:0] null8;
        case(ir[5:3])
            0: // ADD ADI
                begin
                    {cac, null4} = acc + sr;
                    {cc, acc} = {1'b0, acc} + sr;
                    calpsz(acc);
                end

            1: // ADC ACI
                begin
                    {cac, null4} = acc + sr + cc;
                    {cc, acc} = {1'b0, acc} + sr + cc;
                    calpsz(acc);
                end

            2: // SUB SUI
                begin
                    {cac, null4} = acc - sr;
                    {cc, acc} = {1'b0, acc} - sr;
                    calpsz(acc);
                end

            3: // SBB SBI
                begin
                    {cac, null4} = acc - sr - cc;
                    {cc, acc} = {1'b0, acc} - sr - cc;
                    calpsz(acc);
                end

            4: // ANA ANI
                begin
                    acc = acc & sr;
                    cac = 1;
                    cc = 0;
                    calpsz(acc);
                end

            5: // XRA XRI
                begin
                    acc = acc ^ sr;
                    cac = 0;
                    cc = 0;
                    calpsz(acc);
                end

            6: // ORA ORI
                begin
                    acc = acc | sr;
                    cac = 0;
                    cc = 0;
                    calpsz(acc);
                end

            7: // CMP CPI
                begin
                    {cac, null4} = acc - sr;
                    {cc, null8} = {1'b0, acc} - sr;
                    calpsz(null8);
                end
        endcase
    endtask





    /* rotate acc and special instructions */
    task racc_spec;
        case(ir[5:3])
            0: // RLC
                begin
                    acc = {acc[6:0], acc[7]};
                    cc = acc[7];
                end

            1: // RRC
                begin
                    acc = {acc[0], acc[7:1]};
                    cc = acc[0];
                end

            2: // RAL
                {cc, acc} = {acc, cc};

            3: // RAR
                {acc, cc} = {cc, acc};

            4: // DAA, decimal adjust
                begin
                    if((acc[3:0] > 9) || cac) acc = acc + 6;
                    if((acc[7:4] > 9) || cc) {cc, acc} = {1'b0, acc} + 'h60;
                end

            5: // CMA
                acc = ~acc;

            6: // STC
                cc = 1;

            7: // CMC
                cc = ~cc;
        endcase
    endtask


    /* increment and decrement register pair */
    task inx_dcx;
        case(ir[5:3])
            0: {regb, regc} = {regb, regc} + 1; // INX B
            1: {regb, regc} = {regb, regc} - 1; // DCX B
            2: {regd, rege} = {regd, rege} + 1; // INX D
            3: {regd, rege} = {regd, rege} - 1; // DCX D
            4: {regh, regl} = {regh, regl} + 1; // INX H
            5: {regh, regl} = {regh, regl} - 1; // DCX H
            6: sp = sp + 1;                     // INX SP
            7: sp = sp - 1;                     // DCX SP
        endcase
    endtask





    /* load register pair immediate */
    task lrpi;
        case(ir[5:4])
            0: adread({regb, regc}); // LXI B
            1: adread({regd, rege}); // LXI D
            2: adread({regh, regl}); // LXI H
            3: adread(sp);           // LXI SP
        endcase
    endtask


    /* add into regh, regl pair */
    task addhl;
    begin
        case(ir[5:4])
            0: {cc, regh, regl} = {1'b0, regh, regl} + {regb, regc}; // DAD B
            1: {cc, regh, regl} = {1'b0, regh, regl} + {regd, rege}; // DAD D
            2: {cc, regh, regl} = {1'b0, regh, regl} + {regh, regl}; // DAD H
            3: {cc, regh, regl} = {1'b0, regh, regl} + sp;           // DAD SP
        endcase
        holdreq;
        holdreq;
    end
    endtask



    /* store and load instruction */
    task sta_lda;
    reg[15:0] ra;
        case(ir[5:3])
            0: memwrite(acc, {regb, regc}); // STAX B
            1: memread(acc, {regb, regc});  // LDAX B
            2: memwrite(acc, {regd, rege}); // STAX D
            3: memread(acc, {regd, rege});  // LDAX D

            4: // SHLD
                begin
                    adread(ra);
                    memwrite(regl, ra);
                    memwrite(regh, ra + 1);
                end
            5: // LHLD
                begin
                    adread(ra);
                    memread(regl, ra);
                    memread(regh, ra + 1);
                end

            6: // STA
                begin
                    adread(ra);
                    memwrite(acc, ra);
                end
            7: // LDA
                begin
                    adread(ra);
                    memread(acc, ra);
                end
        endcase
    endtask



    /* push register pair from stack */
    task push;
        case(ir[5:4])
            0: push2b(regb, regc); // PUSH B
            1: push2b(regd, rege); // PUSH D
            2: push2b(regh, regl); // PUSH H
            3: push2b(acc, {cs,cz,1'b1,cac,1'b1,cp,1'b1,cc}); // PUSH PSW
        endcase
    endtask

    /* push 2 bytes onto stack */
    task push2b;
    input[7:0] highb, lowb;
    begin
        sp = sp - 1;
        memwrite(highb, sp);
        sp = sp - 1;
        memwrite(lowb, sp);
    end
    endtask


    /* pop register pair from stack */
    task pop;
    reg null1;
        case(ir[5:4])
            0: pop2b(regb, regc); // POP B
            1: pop2b(regd, rege); // POP D
            2: pop2b(regh, regl); // POP H
            3: pop2b(acc,
                {cs, cz, null1, cac, null1, cp, null1, cc}); // POP PSW
        endcase
    endtask

    /* pop 2 bytes from stack */
    task pop2b;
    output[7:0] highb, lowb;
    begin
        memread(lowb, sp);
        sp = sp + 1;
        memread(highb, sp);
        sp = sp + 1;
    end
    endtask




    /* check hold request */
    task holdreq;
    begin
        aleff = 0;
        s0ff = 0;
        s1ff = 1;
        iomff = 0;
        addr = pc;
        if(hold) begin
            holdff = 1;
            acontrol = 0;
            dcontrol = 0;
            @ec2 hldaff = 1;
        end
        else begin
            acontrol = 1;
            dcontrol = 1;
        end
        @ec1 dcontrol = 0;
        @ec1 @ec2;
    end
    endtask




    /* conditional jump, call and return instructions */
    task condjcr;
    reg branch;
    begin
        case(ir[5:3])
            0: branch = !cz; // JNZ CNZ RNZ
            1: branch = cz;  // JZ  CZ  RZ
            2: branch = !cc; // JNC CNC RNC
            3: branch = cc;  // JC  CC  RC
            4: branch = !cp; // JPO CPO RPO
            5: branch = cp;  // JPE CPE RPE
            6: branch = !cs; // JP  CP  RP
            7: branch = cs;  // JM  CM  RM
        endcase
        if(branch)
            case(ir[2:0])
                0: // return
                    pop2b(pc[15:8], pc[7:0]);

                2: // jump
                    adread(pc);

                4: // call
                    begin :call
                        reg [15:0] newpc;
                        adread(newpc);
                        push2b(pc[15:8], pc[7:0]);
                        pc = newpc;
                    end

                default no_instruction;
            endcase
        else
            case(ir[2:0])
                0: ;
                2, 4:
                    begin
                        memread(data, pc);
                        pc = pc + 2;
                    end
                default no_instruction;
            endcase
    end
    endtask




    /* restart instructions */
    task restart;
    begin
        push2b(pc[15:8], pc[7:0]);
        case(ir[5:3])
            0: pc = 'h00; // RST 0
            1: pc = 'h08; // RST 1
            2: pc = 'h10; // RST 2
            3: pc = 'h18; // RST 3
            4: pc = 'h20; // RST 4
            5: pc = 'h28; // RST 5
            6: pc = 'h30; // RST 6
            7: pc = 'h38; // RST 7
        endcase
    end
    endtask




    /* new instructions - except for NOP */
    task newops;
        case(ir[5:3])
            0: ; // NOP

            4: // RIM
                begin
                    acc = {sid, intmask[7:5], intmask[3:0]};
                    if(trapi) begin
                        intmask[3] = inte;
                        trapi = 0;
                    end
                end

            6: // SIM
                begin
                    if(acc[3]) begin
                        intmask[2:0] = acc[2:0];
                        intmask[6:5] = intmask[6:5] & acc[1:0];
                    end
                    intmask[8] = acc[4];
                    if(acc[6]) @ec1 @ec1 @ec2 sodff = acc[7];
                end

            default no_instruction;
        endcase
    endtask



    /* decode 1 instructions */
    task decode1;
        case(ir[5:4])
            0: pop2b(pc[15:8], pc[7:0]); // RET
            2: pc = {regh, regl}; // PCHL
            3: sp = {regh, regl}; // SPHL
            default no_instruction;
        endcase
    endtask



    /* decode 2 instructions */
    task decode2;
    reg[7:0] saveh, savel;
        case(ir[5:3])
            0: adread(pc); // JMP

            2: // OUT
                begin
                    memread(data, pc);
                    pc = pc + 1;
                    iowrite(data);
                end

            3: // IN
                begin
                    memread(data, pc);
                    pc = pc + 1;
                    ioread(data);
                end

            4: // XTHL
                begin
                    saveh = regh;
                    savel = regl;
                    pop2b(regh, regl);
                    push2b(saveh, savel);
                end

            5: // XCHG
                begin
                    saveh = regh;
                    savel = regl;
                    regh = regd;
                    regl = rege;
                    regd = saveh;
                    rege = savel;
                end

            6: // DI, disable interrupt
                {intmask[6:5], intmask[3]} = 0;

            7: // EI, enable interrupt
                intmask[3] = 1;

            default no_instruction;
        endcase
    endtask


    /* decode 3 instructions */
    task decode3;
        case(ir[5:4])
            0: // CALL
                begin :call
                    reg [15:0] newpc;
                    adread(newpc);
                    push2b(pc[15:8], pc[7:0]);
                    pc = newpc;
                end

            default no_instruction;
        endcase
    endtask


    /* fetch address from pc+1, pc+2 */
    task adread;
    output[15:0] address;
    begin
        memread(address[7:0], pc);
        pc = pc + 1;
        memread(address[15:8], pc);
        if(!int) pc = pc + 1;
    end
    endtask


    /* calculate cp cs and cz */
    task calpsz;
    input[7:0] tr;
    begin
        cp = ^tr;
        cz = tr == 0;
        cs = tr[7];
    end
    endtask


    /* undefined instruction */
    task no_instruction;
    begin
        $display("Undefined instruction");
        dumpstate;
        $finish;
    end
    endtask


    /* print the state of the 8085a */
    task dumpstate;
    begin
        $write(
            "\nDUMP OF 8085A REGISTERS\n",
            "acc=%h regb=%h regc=%h regd=%h rege=%h regh=%h regl=%h\n",
                acc, regb, regc, regd, rege, regh, regl,
            "cs=%h cz=%h cac=%h cp=%h cc=%h\n",
                cs, cz, cac, cp, cc,
            "pc=%h sp=%h addr=%h ir=%h data=%h\n",
                pc, sp, ir, addr, data,
            "intmask=%h aleff=%h s0ff=%h s1ff=%h hldaff=%h holdff=%h\n",
                intmask, aleff, s0ff, s1ff, hldaff, holdff,
            "intaff=%h trapff=%h trapi=%h inte=%h int=%h validint=%h\n",
                intaff, trapff, trapi, inte, int, validint,
            "haltff=%h resetff=%h clockff=%h sodff=%h\n",
                haltff, resetff, clockff, sodff,
            "read=%h write=%h iomff=%h acontrol=%h dcontrol=%h s=%h\n",
                read, write, iomff, acontrol, dcontrol, s,
            "clock=%h x2=%h sid=%h trap=%h rst7p5=%h rst6p5=%h rst5p5=%h\n",
                clock, x2, sid, trap, rst7p5, rst6p5, rst5p5,
            "intr=%h nreset=%h hold=%h ready=%h a=%h ad=%h\n\n",
                intr, nreset, hold, ready, a, ad,
            "instructions executed = %d\n\n", instruction);
    end
    endtask


endmodule /* of i85 */




//Programmable communication interface
module i8251(dbus,rcd,gnd,txc_,write_,chipsel_,comdat_,read_,rxrdy,
             txrdy,syndet,cts_,txe,txd,clk,reset,dsr_,rts_,dtr_,rcc_,vcc);

    parameter [7:0] instance_id = 8'h00;

    reg [8:1] dflags;
    initial   dflags = 'b011;
    // diag flags:
    // 1 = print transmitting
    // 2 = print receiving

    input
        rcd, // receive data
        rcc_, // receive clock
        txc_, // transmit clock
        chipsel_, // chip selected when low
        comdat_,  // command/data_ select
        read_, write_,
        dsr_, // data set ready
        cts_, // clear to send
        reset,// reset when high
        clk,
        gnd,
        vcc;

    output
        rxrdy, // receive data ready when high
        txd, // transmit data line
        txrdy, // transmit buffer ready to accept another byte to transfer
        txe, // transmit buffer empty
        rts_, // request to send 
        dtr_; // data terminal ready

    inout[7:0]
        dbus;

    inout
        syndet; //outside synchonous detect or output to indicate syn det

    supply0
        gnd;
    supply1
        vcc;

    reg
        txd,
        txrdy,
        rxrdy,
        txe,
        dtr_,
        rts_;

    reg[7:0]
        receivebuf,
        status;

    reg
        recvdrv,
        statusdrv;

    assign
        // if recvdrv 1 dbus is driven by rec
        dbus = recvdrv ? receivebuf : 8'bz,
        dbus = statusdrv ? status : 8'bz;

    reg[7:0]
        command,
        transmbuf,
        sync1, sync2, // synchronous data bytes
        tdata, // transmit data register
        modreg;

    // temporary registers
    reg[1:0] csel;
    reg[5:0]
        baudmx,
        tbaudcnt,
        rbaudcnt; // baud rate 
    reg[7:0]
        tstoptotal; // total no. of tranmit clock pulses for stop bit
    reg[3:0]
        databits,
        tdatacount,
        rdatacount; // number of data bits in a character

    reg
        rdatain, // a data byte is read in if 1
        tdatain; // one byte in the transmit reg if 1

    event
        resete,
        rcvrdy;

    initial
        begin // power-on reset
            ->resete;
        end

    always @reset
        if(reset)
            begin
                // external reset
                -> resete;
            end

    always @resete
        begin
            $display("Performing 8251(%m) reset sequence");
            csel=0;
            tdatain=0;
            rdatain=0;
            status=5;
            statusdrv=0;
            recvdrv=0;
            txd=1; // line at mark state when no transmit data
            // assign not allowed for status, txrdy, etc.
            txrdy=1;
            rxrdy=0;
            txe=1;
            command=0;
            disable trans1;
            disable trans2;
            disable trans21;
            disable trans3;
            disable trans4;
            disable trans5;
            disable trans6;
            disable receive;
        end

    /* read in the command/data, and transmit the status/data */
    event
        readope,
        wdatardy;

    always @(negedge read_) 
        if (chipsel_==0)
            begin
                if (comdat_==0)
                    begin
                        recvdrv=1;
                        rdatain=0; // no receive byte is ready
                        rxrdy=0;
                        status[1]=0;
                    end
                else
                    statusdrv=1;

                -> readope;
            end

    always @readope @(posedge read_)
        begin
            recvdrv=0;
            statusdrv=0;
        end

    always @(posedge write_)
    if (chipsel_==0) begin  // read the command/data from the computer
        if (comdat_==0) begin
            transmbuf=dbus;
            txrdy=0; // transmit buffer not ready after receiving data
            txe=0;
            status=status & 8'b11111010;
            #20 // to guarantee txrdy pulse of 20
            if(tdatain==0) begin // move the byte to transmit reg
                tdata=transmbuf;
                tdatain=1;
                txrdy=1;
                status[0]=1;
                if(command[0]) ->wdatardy;
                end
            end
        else begin
            case (csel)
                0: begin modreg=dbus; 
                    if(modreg[1:0]==0)csel=1;//syn
                    else begin // asynchronous op
                        csel=3;
                        baudmx=1; // 1X baud rate
                        if(modreg[1:0]==2'b10)baudmx=16;
                        if(modreg[1:0]==2'b11)baudmx=64;
                        // set up the stop bits in clocks
                        tstoptotal=baudmx;
                        if(modreg[7:6]==2'b10)tstoptotal=
                                   tstoptotal+baudmx/2;
                        if(modreg[7:6]==2'b11)tstoptotal=
                                   tstoptotal+tstoptotal;
                        end
                    databits=modreg[3:2]+5; // bits per char
                    end
                1: begin sync1=dbus; 
                    if(modreg[7]==0)csel=2;// 2 syn bytes
                    else csel=3;
                    end
                2: begin sync2=dbus;
                    csel=3;
                    end
                3: begin command=dbus;
                    if(command[0] && tdatain) -> wdatardy;
                    if(command[1])dtr_=0;
//                    if(command[3]) assign txd=0;
//                    else deassign txd;
                    if(command[4]) status[5:3]=0;
                    if(command[5]) rts_=0;
                    if(command[6]) ->resete;
                    csel=0;
                    -> rcvrdy;
                    end
                endcase  
            end
        end

    /* transmit operation, serial data is sent out at
       the negative edge of the clock  */
    event
        tdatape,
        tbaude,
        tendbaude,
        tstope,
        tstopfe,
        txende,
        tbx2;

    always @wdatardy @(negedge txc_) begin :trans1
        if(dflags[1])
            $display("8251(%h) transmitting data: %b",
                instance_id, tdata);
        tdatacount=databits;
        txd=0; // send out the start bit
        tbaudcnt=baudmx;
        -> tdatape; // ready to transmit data
        -> tbaude;  // to count clocks in a bit
        end

    always @tbaude @(negedge txc_) begin :trans2 
        // count the clocks in a bit period
        tbaudcnt=tbaudcnt-1;
        if (tbaudcnt==0) ->tendbaude;
        else ->tbx2;
        end

    always @tbx2 begin :trans21
        #1 ->tbaude;
        end
    always @tdatape begin :trans3
        while(tdatacount>0) begin
            @tendbaude txd=tdata[0];
            tdata=tdata>> 1;
            tbaudcnt=baudmx;
            tdatacount=tdatacount-1;
            ->tbaude;
            end
        -> tstope;
        end
    always @tstope @tendbaude begin :trans4
        // send out the stop bit, note stop periods
        // are counted in terms of clock pulses
        txd=1;
        ->tstopfe;
        end
    always @tstopfe begin :trans5
        // transmit the stop bit for the clock periods
        repeat(tstoptotal) @(negedge txc_);
        ->txende;
        end
    always @txende begin :trans6
        // end of the data byte transmitted
        if(txrdy==0) begin
            tdata=transmbuf;
            txrdy=1;
            status[0]=1;
            if(command[0]) ->wdatardy;
            end
        else tdatain=0;
        end

    /* receive operation, data is received at the leading edge of the clock */
    always @rcvrdy receive;
    task receive;
    forever
        @(negedge rcd)
            begin
            // the receive line goto zero, maybe a start bit 
            rbaudcnt = baudmx / 2;
            repeat(rbaudcnt) @(negedge rcc_);
            // at the end of half bit, check still start bit
            if (rcd == 0) // check if noise then prepare to receive
                begin
                    rbaudcnt=baudmx;
                    rxrdy=0; // turn-off the read byte ready signal
                    status[1]=0;
                    repeat(databits)
                        begin
                            repeat(rbaudcnt) @(negedge rcc_);
                            // read in the data bit
                            // shift the data in to MSB
                            #1 receivebuf= {rcd, receivebuf[7:1]};
                        end
                    // shift the data to the low part
                    receivebuf=receivebuf >> (8-databits);

                    repeat(rbaudcnt) @(negedge rcc_);

                    // check the stop bit about frame error, etc.
                    #1 if (rcd==0)
                        begin
                            status[5]=1;
                            $display("8251(%h) receive frame error at time %d",
                                instance_id, $time);
                        end
                    else
                        begin
                            // data byte is read in
                            if(dflags[2])
                                $display("8251(%h) received data: %b",
                                    instance_id, receivebuf);
$stop;
                            if (modreg[4]==1)
                                // check the parity of the receive byte
                                status[3]= ^receivebuf ^~ modreg[5];
                            if (command[2]==1)
                                begin
                                    rxrdy=1;
                                    status[1]=1;
                                end
                            if (rdatain==1) status[4]=1; // set overrun error
                            rdatain=1;
                        end
                end
            end
    endtask
endmodule



module test8251(txd, rxd); 
    input txd;
    output rxd;
    reg  read_, write_, chipsel_, reset, comdat_;
    reg[7:0] dbus;
    tri[7:0] dbus1 = dbus;
    reg  txc_;
    wire txd, rxd, txrdy, rxrdy;

    i8251
        p2 (dbus1,txd,,txc_,write_,chipsel_,comdat_,read_,rxrdy,
            txrdy,,,,rxd,,reset,,,,txc_,);
    defparam p2.instance_id = 8'h02;

    /* apply the test waveform */
    initial
        begin
            #1 read_=1;
               write_=1;
               chipsel_=1;
               reset=1;
            #5 reset=0;
            #10 dbus=8'b01001101; // aynch 16X, 8-bits, no-parity, 1 stop
            #10 chipsel_=0;
            #10 comdat_=1;
            #20 write_=0;
            #10 write_=1;
            #10 dbus=5; // enable tx, enable receive
            #10 write_=0;
            #10 write_=1;
            #5  dbus='bZ;
        end

    reg [7:0] senddata;
    reg sendflag;

    task send;
        input [7:0] char;
        begin
            senddata=char;
            sendflag=1;
        end
    endtask

    initial #500
        begin :wav1
            sendflag = 0;

            forever wait(sendflag)
                begin
                    /* send in a byte when ready */
                    wait(txrdy)
                        begin
                            comdat_=0;
                            chipsel_=0;
                            dbus=senddata;
                            #10 write_=0;
                            #20 write_=1;
                            $display("sending byte: %b %h %c",
                                dbus,dbus,dbus);
                            sendflag=0;
                            @txrdy; // wait until txrdy has been changed
                            #20 chipsel_=1;
                            dbus='bZ;
                        end
                end
        end

    // display the received byte
    always wait(rxrdy)
        begin :recmon // wait until the data byte is ready
            reg[7:0] recdata;
            chipsel_=0;
            comdat_=0;
            read_=0;
            #20 recdata=dbus1;
                read_=1;
            #20 chipsel_=1;
            $display("        display byte: %b %h %c",
                recdata, recdata, recdata);
        end

    // generate the transmitter clock
    initial #100
        begin :pclockwave
            forever
                begin
                    #pc.pchperiod txc_ = 1;
                    #pc.pchperiod txc_ = 0;
                end
        end
endmodule

module pc; // serial interface clock period definition
    reg [15:0] pchperiod;
    initial pchperiod = 5000;
endmodule

