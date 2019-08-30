`timescale 1 ps / 1 ps
// linked list
// 255 - 240 | 239 - 224 | 223 - 208 | 207 - 192 | *******|127 - 0|
// next addr | comm		 |  src	    | tag		 | ******| data  |
// input request
// output match signal and message
// input message 

//message from router: |127:123| 122:120|119:88|
//							  |eager  | opcode |mpitype, communicator, src, dst|

`define NEXT_ADDR 255:240
`define COMM		239:224
`define SRC			223:208
`define TAG			207:192
`define MESSAGE   127:0
`define NULL 		16'b0
module linked_list
#(
parameter packetizer_width=128, 
parameter DATA_WIDTH=256, 
parameter ADDR_WIDTH=10) 
(
   input [31:0] request,    //from read request
	input [packetizer_width-1 : 0]  message,  //input from network
	input find,
	input insert,
	output reg found, 
	output reg not_found,
	output reg Q_full,
	output reg [packetizer_width-1:0] unexpected_message,
	input  clk, 
	input  rst,
	input drain
	);
	//'b0 address is NULL
	reg [DATA_WIDTH-1:0] list[2**ADDR_WIDTH-1:0];
	
	//unexpected queue head and tail ptrs
	reg [ADDR_WIDTH-1:0] UMQ_head, UMQ_tail;
	reg [ADDR_WIDTH-1:0] freeQ_head, freeQ_tail;
	//keep track of the queue condition
	integer count, i;
	wire Q_empty;
	assign Q_empty = (count == 'b0);
	always @ (posedge clk) begin
		if (rst) begin
			count = 0;			
			Q_full = 'b0;
		end else if (count == 2**ADDR_WIDTH-1) begin
	      count = count;
			Q_full = 'b1;
		end else begin
			if (insert) begin
				count = count + 1;
			end
			if (find) begin
				count = count - 1;
			end
		end
	end
	
	reg [ADDR_WIDTH-1:0] ptr, ptr_prev;
	//inserting when receive from network
	always @ (posedge clk) begin
		if (rst) begin
			UMQ_head <= 'b0;
			UMQ_tail <= 'b0;
			freeQ_head <= 'b0;
			freeQ_tail <= 16'b1111111111111111;
			for (i = 0; i < 2**ADDR_WIDTH - 1; i = i + 1) begin
				list[i][`NEXT_ADDR] = i + 1;
				list[i][239:0] = 'b0;  
			end
			list[2**ADDR_WIDTH - 1] = 'b0; //corner case, the end points to NULL
		end else begin
			if (insert) begin
				//many corner cases
				//if the UMQ is current empty, add message in head
				if(Q_empty) begin
					list['b1] <= {`NULL,{8'b0,message[111:104]},{8'b0,message[103:96]}, {8'b0,message[95:88]}, 64'b0,message};
					UMQ_head <= 'b1;
					UMQ_tail <= 'b1;
					freeQ_head <= 16'd2;
				end else if (count == 1) begin
					list[{6'b0,UMQ_head}][`NEXT_ADDR] <= freeQ_head;
					list[{6'b0,freeQ_head}]<= {`NULL,{8'b0,message[111:104]},{8'b0,message[103:96]},{8'b0,message[95:88]}, 64'b0,message};
					freeQ_head <= list[{6'b0,freeQ_head}][`NEXT_ADDR];//change  pointer   
					UMQ_tail <= freeQ_head;
				end else begin
					list[{6'b0,freeQ_head}]<= {`NULL,{8'b0,message[111:104]},{8'b0,message[103:96]}, {8'b0,message[95:88]},64'b0,message};
					freeQ_head <= list[{6'b0,freeQ_head}][`NEXT_ADDR];//change  pointer   
					UMQ_tail <= freeQ_head;
				end
			end
			//deletion
			else if (found) begin
				if (ptr_prev == `NULL) begin
					UMQ_head <= list[{6'b0,ptr}][`NEXT_ADDR];
					list[{6'b0,ptr}][`NEXT_ADDR] <= `NULL;
					list[{6'b0,freeQ_tail}][`NEXT_ADDR] <= {6'b0,ptr};
					freeQ_tail <= ptr;
				end else begin
				   list[{6'b0,ptr_prev}][`NEXT_ADDR] <= list[{6'b0,ptr}][`NEXT_ADDR];
					list[{6'b0,ptr}][`NEXT_ADDR] <= `NULL;
					list[{6'b0,freeQ_tail}][`NEXT_ADDR] <= {6'b0,ptr};
					freeQ_tail <= ptr;
				end				
			end
		end
	
	end
	/**************FSM for data searching***********************/
	wire match;
	
	reg state;
	localparam IDLE = 'b0,
				  FINDING = 'b1;
				  
	assign match = (list[{6'b0,ptr}][`COMM] == {8'b0,request[23:16]}) && 
				(list[{6'b0,ptr}][`SRC] == {8'b0,request[15:8]})  && 
				(list[{6'b0,ptr}][`TAG] == {8'b0,request[7:0]});

	
	
	always @ (posedge clk) begin
		if (rst) begin
		  found <= 'b0;
		  unexpected_message <= 'b0;
		  not_found <= 'b0;
		end else begin
			case (state) 
				IDLE: begin
				   found <= 'b0;
					unexpected_message <= 'b0;
					not_found <= 'b0;
					if (find) begin
						state <= FINDING;
						ptr <= UMQ_head;
						ptr_prev <= `NULL;
					end
				end
				FINDING: begin
				   if (count == 0 ) begin //&& (!drain)) begin
						found <= 'b0;
						unexpected_message <= 'b0;
						state <= IDLE;
					end else begin
					   if (match) begin
							found <= 'b1;
							unexpected_message <= list[{6'b0,ptr}][`MESSAGE];
							state <= IDLE;
						end else begin
							if (list[{6'b0,ptr}][`NEXT_ADDR] != `NULL) begin
								ptr <= list[{6'b0,ptr}][`NEXT_ADDR];
								ptr_prev <= ptr;
							end else begin
								//if (!drain) begin
									found <= 'b0;
									unexpected_message <= 'b0;
									state <= IDLE;
									not_found <= 'b1;
								//end else if (drain) begin
								//	state <= FINDING; //hold here and wait for the next one								
								//end		 					
							end
						end
					end
				end
				default: begin
					state <= IDLE;
				end							
		   endcase
		end	
	end
	
	
	
endmodule
	
	
	
	
	