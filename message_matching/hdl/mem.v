`timescale 1 ps / 1 ps
module mem #(
		parameter cpu_width = 32,
		parameter packetizer_width = 128,
		parameter data_width = 32,
		parameter mem_width = 32,
		parameter mem_depth = 11,
		parameter threshold  = 16,
		parameter SIZE = 3
)(
	   input wire  clk,
		input wire reset,
		input wire hold,
		input wire [data_width-1:0] read_addr,       //21~11: address, 10:0 size
		input wire read_mem_valid,
		output wire [data_width-1:0] data,		
		output reg [10:0] tag,
		////////////////////////////////////connectiing to the main mem//////////////////////////////////////////////
		output wire chipselect,								//this one goes into the main mem
		output wire [19:0] mem_addr,						//this one goes into the main mem of each processor
		input wire [data_width-1:0] data_from_mem
		
);

reg start;
reg end_read;
reg [10:0] size;
reg [10:0] base_addr, addr_local; 

assign mem_addr = {9'b0,addr_local};
assign chipselect = read_mem_valid || (~end_read);
assign data = data_from_mem;

/* when there is a posedge of read memory valid signal, hold start for the size of the message*/
always @ (posedge read_mem_valid)
	if (reset) begin
		start = 1'b0;
	end 
	else if (read_mem_valid)begin
		start = 1'b1;
	end 
	else begin
		start = 1'b0;
	end
	
always @ (start, reset) 
	if (reset) begin
		size = 11'b0;
		base_addr = 11'b0;
	end
	else if (start) begin
		size = read_addr[10:0]; 
		base_addr = read_addr[21:11];
	end
	
always @ (posedge clk) 
begin : TAG_GENERATOR
	if (reset) begin
			tag <= 11'b0;
			addr_local <= 11'b0;
			end_read <= 1'b1;
	end 
	else if (start && (tag < size-1) && (~hold)) begin
			end_read <= 1'b0;
			tag <= tag + 11'b1;
			addr_local <= base_addr + tag<<2;
	end else if (start && (tag == size -1)) begin
			end_read <= 1'b1;
			tag <= 11'b0;
			addr_local <= 11'b0; 
	end
end 





endmodule 