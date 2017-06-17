
//Primitive name and terminal list
primitive udp_and(out, a, b);

//Declarations
output out; //must not be declared as reg for combinational UDP
input a, b; //declarations for inputs.

//State table definition; starts with keyword table
table
	//The following comment is for readability only
	//Input entries of the state table must be in the
	//same order as the input terminal list.
   // a   b   :   out;
      0   0   :   0; 
      0   1   :   0;
      1   0   :   0;
      1   1   :   1;

endtable //end state table definition

endprimitive //end of udp_and definition



