File Name		Description		Reference in Book
---------		-----------		-----------------

mag_comp.v		RTL description of a	Section 14.4.2
			4-bit magnitude 
			comparator.

mag_comp.gv		gate level description 	Section 14.4.2
			of a 4-bit magnitude 
			comparator.

stimulus.v		Stimulus file to verify Section 14.5.1
			the functionality of
			gate vs. RTL description.

abc_100.v		Simulation library for	Section 14.5.1
			abc_100 technology cells.

abc_100.db		Synthesis library for
			abc_100 technology cells.
			This is in a Synopsys
			format. (not included)


To run the RTL level simulation type, files needed are

mag_compare.v	 stimulus.v, and  abc_100.v(library file)


To run the gate level simulation type, files needed are

mag_compare.gv,  stimulus.v, and  abc_100.v(library file)



