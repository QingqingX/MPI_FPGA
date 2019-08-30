/* return only the first match when it is wildcard*/
`timescale 1 ps / 1 ps
module encoder
(
   input [255:0] in,    //from read request
	input clk,
	output reg [7:0] number
	);
  
integer i;

always @ (*) begin
      number = 'b0;
		for (i = 0; i < 256; i = i + 1) begin
			if (in[i] == 1'b1 ) begin			
				number =  i;	
			end
		end
	
end

endmodule
