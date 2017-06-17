File Name		Description		Reference in Book
---------		-----------		-----------------

vend.v			RTL description of a	Section 14.7.4
			FSM for a newspaper
			vending machine.

vend.gv			gate level description 	Section 14.7.8
			of a FSM for a newspaper 
			vending machine.

vendtest.v		Stimulus file to verify Section 14.7.9
			the functionality of
			gate vs. RTL description.

abc_100.v		Simulation library for	Section 14.7.5
			abc_100 technology cells.

abc_100.db		Synthesis library for
			abc_100 technology cells.
			This is in a Synopsys
			format.


To run the RTL level simulation type, files needed are

vend.v	 vend_test.v, and  abc_100.v(library file)


To run the gate level simulation type, files needed are

vend.gv,  vend_test.v, and  abc_100.v(library file)



