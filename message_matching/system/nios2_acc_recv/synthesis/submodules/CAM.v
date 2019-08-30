// Content addressable memory
// True Dual Port RAM with single clock
// port a for write by network
// port b for read by request
// store the whole packet from router, and attach tag at the begining
// tag:  read: 0 // no data here
//		   read: 1 // data here, after output the data, mark it 0
 
module CAM
#(parameter packetizer_width=128, DATA_WIDTH=256, parameter ADDR_WIDTH=2)
(
	input [(packetizer_width-1):0] data_a, data_b,
	input [(ADDR_WIDTH-1):0] addr_a, addr_b,
	input we_a, re_b, clk, rst,
	output reg [(DATA_WIDTH-1):0] q_a,
	output reg [packetizer_width -1 : 0] q_b
);


parameter NO_DATA=1'b0;
parameter VALID_DATA=1'b1;

	// Declare the RAM variable
	//!!!!!!!!!!!!!!!!restrictions: only one send with one receive paired will work
	//if there are many eager sends before receives,CAM just overflows 
	reg [DATA_WIDTH-1:0] ram[2**ADDR_WIDTH-1:0];

	integer x;
	// Port A for network to write
	always @ (posedge clk)
	begin
	   if (rst) begin
		   for (x = 0 ; x < (2**ADDR_WIDTH) ; x = x + 1)
				ram[x] <= 'b0;
			q_a <= 'b0;
		end 
		//when there is a data from network,check the highest bit,
	   //if it is 0, write to the correct location and rise the higheset bit
		//if it is 1, dont write (needs optimization!!!)
		//later we need more locations to manage many data from one src
		else if (we_a) 
		begin
		   if (ram[addr_a][DATA_WIDTH-1] == 1'b0)  begin //if no valid data here
				ram[addr_a] <= {VALID_DATA, 127'b0, data_a};   
				q_a <= 'b0;
			end
		end
		//2)when there is a read
		//wait for the write to finish
		else if (~we_a && re_b) begin
			ram[addr_b][DATA_WIDTH-1] <= NO_DATA;		
		end
		//when no data write, just output the content
		else 
		begin
			q_a <= ram[addr_a];
		end 
	end 

	// Port B only for read 
	always @ (posedge clk)
	begin
		if (rst)  //we_b is read enable
		begin
			q_b <= 'b0;
		end
		//when there is a read request from the data finder
		//1)when there is not a simultaneous write from network
		//check the highest bit, if 1, return data and set this bit invalid
		//if 0, return 0data
		else if (re_b)
		begin
		   if (ram[addr_b][DATA_WIDTH-1] == 1'b1) begin
				q_b <= ram[addr_b][packetizer_width-1:0];				
			end else begin
				q_b <= 'b0;
			end
		end 
	end

	
endmodule
