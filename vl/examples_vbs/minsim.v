/* from "The Verilog Hardware Description Language" by Thomas and Moorby
    Kluwer Academic Publishers, Norwell, MA  (617)871-6300 */
//THE MINISIM EXAMPLE

module miniSim;

// element types being modeled
`define Nand 0
`define DEdgeFF 1
`define Wire 2

// literal values with strength:
//   format is 8 0-strength bits in decreasing strength order
//   followed by 8 1-strength bits in decreasing strength order
`define Strong0 16'b01000000_00000000
`define Strong1 16'b00000000_01000000
`define StrongX 16'b01111111_01111111
`define Pull0   16'b00100000_00000000
`define Pull1   16'b00000000_00100000
`define Highz0  16'b00000001_00000000
`define Highz1  16'b00000000_00000001

// three-valued logic set
`define Val0 3'd0
`define Val1 3'd1
`define ValX 3'd2

parameter
    DebugFlags =               'b11000, //set to 1 for message
//                               |||||
//  loading                   <--+||||
//  event value changes      <----+|||
//  wire calculation        <------+||
//  evaluation             <--------+|
//  scheduling           <-----------+

    IndexSize = 16,     //maximum size for index pointers
    MaxElements = 50,   //maximum number of elements
    TypeSize = 12;      //maximum number of types

reg [IndexSize-1:0]
    eventElement,             //output value change element
    evalElement,              //evaluation element on fanout
    fo0Index[1:MaxElements],  //first fanout index of eventElement
    fo1Index[1:MaxElements],  //second fanout index of eventElement
    currentList,              //current time scheduled event list
    nextList,                 //unit delay scheduled event list
    schedList[1:MaxElements]; //scheduled event list index
reg [TypeSize-1:0]
    eleType[1:MaxElements]; //element type
reg
    fo0TermNum[1:MaxElements],   //first fanout input terminal number
    fo1TermNum[1:MaxElements],   //second fanout input terminal number
    schedPresent[1:MaxElements]; //element is in scheduled event list flags
reg [15:0]
    eleStrength[1:MaxElements], //element strength indication
    outVal[1:MaxElements],      //element output value
    in0Val[1:MaxElements],      //element first input value
    in1Val[1:MaxElements],      //element second input value
    in0, in1, out, oldIn0;      //temporary value storage

integer pattern, simTime; //time keepers

initial
  begin
    // initialize variables
    pattern = 0;
    currentList = 0;
    nextList = 0;

    $display("Loading toggle circuit");
    loadElement(1, `DEdgeFF, 0, `Strong1,0,0, 4,0,0,0);
    loadElement(2, `DEdgeFF, 0, `Strong1,0,0, 3,0,0,0); 
    loadElement(3, `Nand, (`Strong0|`Strong1),
        `Strong0,`Strong1,`Strong1, 4,0,1,0); 
    loadElement(4, `DEdgeFF, (`Strong0|`Strong1),
        `Strong1,`Strong1,`Strong0, 3,0,1,0); 

    // apply stimulus and simulate
    $display("Applying 2 clocks to input element 1");
    applyClock(2, 1);
    $display("Changing element 2 to value 0 and applying 1 clock");
    setupStim(2, `Strong0);
    applyClock(1, 1);

    $display("\nLoading open-collector and pullup circuit");
    loadElement(1, `DEdgeFF, 0, `Strong1,0,0, 3,0,0,0);
    loadElement(2, `DEdgeFF, 0, `Strong0,0,0, 4,0,0,0);
    loadElement(3, `Nand, (`Strong0|`Highz1),
        `Strong0,`Strong1,`Strong1, 5,0,0,0);
    loadElement(4, `Nand, (`Strong0|`Highz1),
        `Highz1,`Strong0,`Strong1, 5,0,1,0);
    loadElement(5, `Wire, 0,
        `Strong0,`Strong0,`Highz1, 7,0,1,0);
    loadElement(6, `DEdgeFF, 0, `Pull1,0,0, 7,0,0,0);
    loadElement(7, `Wire, 0,
        `Strong0,`Pull1,`Strong0, 0,0,0,0);

    // apply stimulus and simulate
    $display("Changing element 1 to value 0");
    pattern = pattern + 1;
    setupStim(1, `Strong0);
    executeEvents;
    $display("Changing element 2 to value 1");
    pattern = pattern + 1;
    setupStim(2, `Strong1);
    executeEvents;
    $display("Changing element 2 to value X");
    pattern = pattern + 1;
    setupStim(2, `StrongX);
    executeEvents;
  end

// Initialize data structure for a given element.
task loadElement;
input [IndexSize-1:0] loadAtIndex; //element index being loaded
input [TypeSize-1:0] type;         //type of element
input [15:0] strengthCoercion;     //strength specification of element
input [15:0] oVal, i0Val, i1Val;   //output and input values
input [IndexSize-1:0] fo0, fo1;    //fanout element indexes
input fo0Term, fo1Term;            //fanout element input terminal indicators
  begin
    if (DebugFlags[4])
        $display(
            "Loading element %0d, type %0s, with initial value %s(%b_%b)",
            loadAtIndex, typeString(type),
            valString(oVal), oVal[15:8], oVal[7:0]);
    eleType[loadAtIndex] = type;
    eleStrength[loadAtIndex] = strengthCoercion;
    outVal[loadAtIndex] = oVal;
    in0Val[loadAtIndex] = i0Val;
    in1Val[loadAtIndex] = i1Val;
    fo0Index[loadAtIndex] = fo0;
    fo1Index[loadAtIndex] = fo1;
    fo0TermNum[loadAtIndex] = fo0Term;
    fo1TermNum[loadAtIndex] = fo1Term;
    schedPresent[loadAtIndex] = 0;
  end
endtask

// Given a type number, return a type string
function [32*8:1] typeString;
input [TypeSize-1:0] type;
    case (type)
      `Nand: typeString = "Nand";
      `DEdgeFF: typeString = "DEdgeFF";
      `Wire: typeString = "Wire";
      default: typeString = "*** Unknown element type";
    endcase
endfunction

// Setup a value change on an element.
task setupStim;
input [IndexSize-1:0] vcElement; //element index
input [15:0] newVal;           //new element value
  begin
    if (! schedPresent[vcElement])
      begin
        schedList[vcElement] = currentList;
        currentList = vcElement;
        schedPresent[vcElement] = 1;
      end
    outVal[vcElement] = newVal;
  end
endtask

// Setup and simulate a given number of clock pulses to a given element.
task applyClock;
input [7:0] nClocks;
input [IndexSize-1:0] vcElement;
    repeat(nClocks)
      begin
        pattern = pattern + 1;
        setupStim(vcElement, `Strong0);
        executeEvents;
        pattern = pattern + 1;
        setupStim(vcElement, `Strong1);
        executeEvents;
      end
endtask

// Execute all events in the current event list.
// Then move the events in the next event list to the current event
// list and loop back to execute these events. Continue this loop
// until no more events to execute.
// For each event executed, evaluate the two fanout elements if present.
task executeEvents;
reg [15:0] newVal;
  begin
    simTime = 0;
    while (currentList)
      begin
        eventElement = currentList;
        currentList = schedList[eventElement];
        schedPresent[eventElement] = 0;
        newVal = outVal[eventElement];
        if (DebugFlags[3])
            $display(
                "At %0d,%0d Element %0d, type %0s, changes to %s(%b_%b)",
                pattern, simTime,
                eventElement, typeString(eleType[eventElement]),
                valString(newVal), newVal[15:8], newVal[7:0]);
        if (fo0Index[eventElement]) evalFo(0);
        if (fo1Index[eventElement]) evalFo(1);
        if (! currentList) // if empty move to next time unit
          begin
            currentList = nextList;
            nextList = 0;
            simTime = simTime + 1;
          end
      end
  end
endtask

// Evaluate a fanout element by testing its type and calling the
// appropriate evaluation routine.
task evalFo;
input fanout; //first or second fanout indicator
  begin
    evalElement = fanout ? fo1Index[eventElement] : 
                           fo0Index[eventElement];
    if (DebugFlags[1])
        $display("Evaluating Element %0d type is %0s",
            evalElement, typeString(eleType[evalElement]));
    case (eleType[evalElement])
      `Nand: evalNand(fanout);
      `DEdgeFF: evalDEdgeFF(fanout);
      `Wire: evalWire(fanout);
    endcase
  end
endtask

// Store output value of event element into
// input value of evaluation element.
task storeInVal;
input fanout; //first or second fanout indicator
  begin
    // store new input value
    if (fanout ? fo1TermNum[eventElement] : fo0TermNum[eventElement])
        in1Val[evalElement] = outVal[eventElement];
    else
        in0Val[evalElement] = outVal[eventElement];
  end
endtask

// Convert a given full strength value to three-valued logic (0, 1 or X)
function [1:0] log3;
input [15:0] inVal;
    casez (inVal)
      16'b00000000_00000000: log3 = `ValX;
      16'b???????0_00000000: log3 = `Val0;
      16'b00000000_???????0: log3 = `Val1;
      default:               log3 = `ValX;
    endcase
endfunction

// Convert a given full strength value to four-valued logic (0, 1, X or Z),
// returning a 1 character string
function [8:1] valString;
input [15:0] inVal;
    case (log3(inVal))
      `Val0: valString = "0";
      `Val1: valString = "1";
      `ValX: valString = (inVal & 16'b11111110_11111110) ? "X" : "Z";
    endcase
endfunction

// Coerce a three-valued logic output value to a full output strength value
// for the scheduling of the evaluation element
function [15:0] strengthVal;
input [1:0] logVal;
    case (logVal)
      `Val0: strengthVal = eleStrength[evalElement] & 16'b11111111_00000000;
      `Val1: strengthVal = eleStrength[evalElement] & 16'b00000000_11111111;
      `ValX: strengthVal = fillBits(eleStrength[evalElement]);
    endcase
endfunction

// Given an incomplete strength value, fill the missing strength bits.
// The filling is only necessary when the value is unknown.
function [15:0] fillBits;
input [15:0] val;
  begin
    fillBits = val;
    if (log3(val) == `ValX)
      begin
        casez (val)
  16'b1???????_????????: fillBits = fillBits | 16'b11111111_00000001;
  16'b01??????_????????: fillBits = fillBits | 16'b01111111_00000001;
  16'b001?????_????????: fillBits = fillBits | 16'b00111111_00000001;
  16'b0001????_????????: fillBits = fillBits | 16'b00011111_00000001;
  16'b00001???_????????: fillBits = fillBits | 16'b00001111_00000001;
  16'b000001??_????????: fillBits = fillBits | 16'b00000111_00000001;
  16'b0000001?_????????: fillBits = fillBits | 16'b00000011_00000001;
        endcase
        casez (val)
  16'b????????_1???????: fillBits = fillBits | 16'b00000001_11111111;
  16'b????????_01??????: fillBits = fillBits | 16'b00000001_01111111;
  16'b????????_001?????: fillBits = fillBits | 16'b00000001_00111111;
  16'b????????_0001????: fillBits = fillBits | 16'b00000001_00011111;
  16'b????????_00001???: fillBits = fillBits | 16'b00000001_00001111;
  16'b????????_000001??: fillBits = fillBits | 16'b00000001_00000111;
  16'b????????_0000001?: fillBits = fillBits | 16'b00000001_00000011;
       endcase
     end
  end
endfunction

// Evaluate a 'Nand' gate primitive.
task evalNand;
input fanout; //first or second fanout indicator
  begin
    storeInVal(fanout);
    // calculate new output value
    in0 = log3(in0Val[evalElement]);
    in1 = log3(in1Val[evalElement]);
    out = ((in0 == `Val0) || (in1 == `Val0)) ?
        strengthVal(`Val1) :
        ((in0 == `ValX) || (in1 == `ValX)) ?
            strengthVal(`ValX):
            strengthVal(`Val0);
    // schedule if output value is different
    if (out != outVal[evalElement])
        schedule(out);
  end
endtask

// Evaluate a D positive edge-triggered flip flop
task evalDEdgeFF;
input fanout; //first or second fanout indicator
    // check value change is on clock input
    if (fanout ? (fo1TermNum[eventElement] == 0) :
                 (fo0TermNum[eventElement] == 0))
      begin
        // get old clock value
        oldIn0 = log3(in0Val[evalElement]);
        storeInVal(fanout);
        in0 = log3(in0Val[evalElement]);
        // test for positive edge on clock input
        if ((oldIn0 == `Val0) && (in0 == `Val1))
          begin
            out = strengthVal(log3(in1Val[evalElement]));
            if (out != outVal[evalElement])
              schedule(out);
          end
      end
    else
        storeInVal(fanout); // store data input value
endtask

// Evaluate a wire with full strength values
task evalWire;
input fanout;
reg [7:0] mask;
  begin
    storeInVal(fanout);

    in0 = in0Val[evalElement];
    in1 = in1Val[evalElement];
    mask = getMask(in0[15:8]) & getMask(in0[7:0]) &
           getMask(in1[15:8]) & getMask(in1[7:0]);
    out = fillBits((in0 | in1) & {mask, mask});

    if (out != outVal[evalElement])
        schedule(out);

    if (DebugFlags[2])
        $display("in0 = %b_%b\nin1 = %b_%b\nmask= %b %b\nout = %b_%b",
            in0[15:8],in0[7:0], in1[15:8],in1[7:0],
            mask,mask, out[15:8],out[7:0]);
  end
endtask

// Given either a 0-strength or 1-strength half of a strength value
// return a masking pattern for use in a wire evaluation.
function [7:0] getMask;
input [7:0] halfVal; //half a full strength value
    casez (halfVal)
      8'b???????1: getMask = 8'b11111111;
      8'b??????10: getMask = 8'b11111110;
      8'b?????100: getMask = 8'b11111100;
      8'b????1000: getMask = 8'b11111000;
      8'b???10000: getMask = 8'b11110000;
      8'b??100000: getMask = 8'b11100000;
      8'b?1000000: getMask = 8'b11000000;
      8'b10000000: getMask = 8'b10000000;
      8'b00000000: getMask = 8'b11111111;
    endcase
endfunction

// Schedule the evaluation element to change to a new value.
// If the element is already scheduled then just insert the new value.
task schedule;
input [15:0] newVal; // new value to change to
  begin
    if (DebugFlags[0])
        $display(
            "Element %0d, type %0s, scheduled to change to %s(%b_%b)",
            evalElement, typeString(eleType[evalElement]),
            valString(newVal), newVal[15:8], newVal[7:0]);
    if (! schedPresent[evalElement])
      begin
        schedList[evalElement] = nextList;
        schedPresent[evalElement] = 1;
        nextList = evalElement;
      end
    outVal[evalElement] = newVal;
  end
endtask
endmodule

/*******************  PLEASE NOTE:  ************************************

The values printed out in the book section 7.1.3 have the wrong
format. The 0 and 1 strength bits need to be swapped around. For
example, the first value printed in the book should read
1(00000000_01000000) and not 1(0100000_00000000). The value format you
get from running the above minisim description through Verilog should
be correct.

Here is a listing of the results:

Loading toggle circuit
Loading element 1, type DEdgeFF, with initial value 1(00000000_01000000)
Loading element 2, type DEdgeFF, with initial value 1(00000000_01000000)
Loading element 3, type Nand, with initial value 0(01000000_00000000)
Loading element 4, type DEdgeFF, with initial value 1(00000000_01000000)
Applying 2 clocks to input element 1
At 1,0 Element 1, type DEdgeFF, changes to 0(01000000_00000000)
At 2,0 Element 1, type DEdgeFF, changes to 1(00000000_01000000)
At 2,1 Element 4, type DEdgeFF, changes to 0(01000000_00000000)
At 2,2 Element 3, type Nand, changes to 1(00000000_01000000)
At 3,0 Element 1, type DEdgeFF, changes to 0(01000000_00000000)
At 4,0 Element 1, type DEdgeFF, changes to 1(00000000_01000000)
At 4,1 Element 4, type DEdgeFF, changes to 1(00000000_01000000)
At 4,2 Element 3, type Nand, changes to 0(01000000_00000000)
Changing element 2 to value 0 and applying 1 clock
At 5,0 Element 1, type DEdgeFF, changes to 0(01000000_00000000)
At 5,0 Element 2, type DEdgeFF, changes to 0(01000000_00000000)
At 5,1 Element 3, type Nand, changes to 1(00000000_01000000)
At 6,0 Element 1, type DEdgeFF, changes to 1(00000000_01000000)

Loading open-collector and pullup circuit
Loading element 1, type DEdgeFF, with initial value 1(00000000_01000000)
Loading element 2, type DEdgeFF, with initial value 0(01000000_00000000)
Loading element 3, type Nand, with initial value 0(01000000_00000000)
Loading element 4, type Nand, with initial value Z(00000000_00000001)
Loading element 5, type Wire, with initial value 0(01000000_00000000)
Loading element 6, type DEdgeFF, with initial value 1(00000000_00100000)
Loading element 7, type Wire, with initial value 0(01000000_00000000)
Changing element 1 to value 0
At 7,0 Element 1, type DEdgeFF, changes to 0(01000000_00000000)
At 7,1 Element 3, type Nand, changes to Z(00000000_00000001)
At 7,2 Element 5, type Wire, changes to Z(00000000_00000001)
At 7,3 Element 7, type Wire, changes to 1(00000000_00100000)
Changing element 2 to value 1
At 8,0 Element 2, type DEdgeFF, changes to 1(00000000_01000000)
At 8,1 Element 4, type Nand, changes to 0(01000000_00000000)
At 8,2 Element 5, type Wire, changes to 0(01000000_00000000)
At 8,3 Element 7, type Wire, changes to 0(01000000_00000000)
Changing element 2 to value X
At 9,0 Element 2, type DEdgeFF, changes to X(01111111_01111111)
At 9,1 Element 4, type Nand, changes to X(01111111_00000001)
At 9,2 Element 5, type Wire, changes to X(01111111_00000001)
At 9,3 Element 7, type Wire, changes to X(01111111_00111111)
*****************************************************************/
