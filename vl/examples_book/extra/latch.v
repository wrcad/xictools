//Define level sensitive latch using UDP.
primitive latch(q, d, clock, clear);

//declarations
output q;
reg q; //q declared as reg
input d, clock, clear;

//sequential UDP initialization
//only one initial statement allowed
initial
    q = 0; 

//state table
table
  //d  clock  clear  : q  :  q+ ;
  
    ?    ?    1      :  ?  : 0  ;  //clear condition; 
                                   //q+ is the new output value
      
    1    1    0      :  ?  :  1 ; //latch q = data  = 1  
    0    1    0      :  ?  :  0 ; //latch q = data  = 0  

    ?    0    0      :  ?  :  - ; //retain original state if clock = 0
endtable

endprimitive
    

