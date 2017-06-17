module file_open;

//Multi-channel descriptor
integer handle1, handle2, handle3; //integers are 32-bit values
integer desc1, desc2, desc3; //three file descriptors

//opening files
//standard output is open; descriptor = 32'h0000_0001 (bit 0 set)
initial
begin
	handle1 = $fopen("file1.out"); //handle1 = 32'h0000_0002 (bit 1 set)
	handle2 = $fopen("file2.out"); //handle2 = 32'h0000_0004 (bit 2 set)
	handle3 = $fopen("file3.out"); //handle3 = 32'h0000_0008 (bit 3 set)

	$display("handle1 = %h, handle2 = %h, handle3 = %h ", 
					handle1, handle2, handle3);

//Writing to files
	desc1 = handle1 | 1; //bitwise or; desc1 = 32'h0000_0003
	$fdisplay(desc1, "Display 1");//write to files file1.out and stdout

	desc2 = handle2 | handle1; //desc2 = 32'h0000_0006
	$fdisplay(desc2, "Display 2");//write to files file1.out and file2.out

	desc3 = handle3 ; //desc3 = 32'h0000_0008
	$fdisplay(desc3, "Display 3");//write to file file3.out only

//Closing files
	$fclose(handle1);
	$fclose(handle2);
	$fclose(handle3);
end



endmodule
