module string_storage;

reg [8*18:1] string_value; // Declare a variable which is 18 bytes wide
initial
    begin
    string_value = "Hello Verilog World"; // String can be stored in variable
    $write("%s", string_value, string_value);
    end

endmodule


