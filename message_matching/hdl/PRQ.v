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
`define REQUEST   31:0
`define NULL 		16'b0
module PRQ
#(
parameter packetizer_width=128, 
parameter DATA_WIDTH=256, 
parameter ADDR_WIDTH=10) 
(
   input [31:0] request,    //from read request
	input [31:0] data_ptr,
	input [packetizer_width-1 : 0]  message,  //input from network
	input find,
	input insert,
	output reg found, 
	output reg not_found,
	output reg Q_full,
	output reg Q_empty,
	output reg [31:0] posted_request,
	input  clk, 
	input  rst
	);
	//'b0 address is NULL
	reg [DATA_WIDTH-1:0] list[2**ADDR_WIDTH-1:0];
	
	//unexpected queue head and tail ptrs
	reg [15:0] UMQ_head, UMQ_tail;
	reg [15:0] freeQ_head, freeQ_tail;
	//keep track of the queue condition
	integer count, i;
	
	always Q_empty = (count == 'b0);
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
	reg [255:0] umq_data,current_data, prev_data, netdata_d, req_data_d;
	wire [255:0] netdata_q,req_data_q; 
	reg [63:0] message_r;
	reg [15:0] ptr, prev, net_w_addr,freeQ_next;
	reg req_r, req_w, net_w, net_r,shift_Q_head_net,  shift_UMQ_head;
	reg [3:0] NET_s;
	//********************************FSM inserting when receive from processor
		localparam IDLE = 'b0001, 
					READ1 = 'b0010,
				  READ2 = 'b0011,
				  INSERT1 = 'b0100;
				 // READ_UMQ2 = 'b0101,
				  //CHK = 'b0110,
				 // READ_prev = 'b0111,
				  //READ_current_2 = 'b1000,
				  
	always @ (posedge clk) begin
		if (rst) begin
			//UMQ_head <= 'b0;
			//UMQ_tail <= 'b0;
			freeQ_head <= 'b1;
			//freeQ_tail <= 16'b1111111111111111;
			net_w <= 'b0;
			freeQ_next <= `NULL;
			net_r <= 'b0;
			net_w_addr <= `NULL;
			message_r <= 'b0;
		end else begin
			case (NET_s) 
			   IDLE: begin
					shift_Q_head_net <= 'b0;
					net_w <= 'b0;
					freeQ_next <= `NULL;
					net_r <= 'b0;
					net_w_addr <= `NULL;
					if (insert) begin 
					   NET_s <= READ1;
						message_r <= {request, data_ptr};
					end
				end
				//read the freeQ head next pointer,assuming it takes one cycle to read
				READ1: begin
				   net_w_addr <= freeQ_head;
					net_r <= 'b1;	
					NET_s <= READ2;
				end
				READ2: begin 
					freeQ_next <= netdata_q[`NEXT_ADDR];
					NET_s <= INSERT1;
				end
				//write to the head of the freeQ and 
				INSERT1: begin
					netdata_d <= {UMQ_head,{8'b0,request[23:16]},{8'b0,request[15:8]}, {8'b0,request[7:0]},128'b0,message_r};
					net_w <= 'b1;
					NET_s <= IDLE;
					net_w_addr <= freeQ_head;
					freeQ_head <= freeQ_next;
					shift_Q_head_net <= 'b1;
					UMQ_newhead_net <= freeQ_head;
				end
				default: begin
					state <= IDLE;
				end	
			endcase
		end
	
	end				
	
	always @ (posedge clk)	begin
		if (rst) begin
			UMQ_head <= `NULL;
		end else begin
			UMQ_head <= shift_UMQ_head ? UMQ_newhead_req : (shift_Q_head_net? UMQ_newhead_net:UMQ_head);
		end	
	end

	/**************FSM for receive request searching***********************/
	wire match;
	reg [15:0] req_addr, UMQ_newhead_req, UMQ_newhead_net;
	reg [3:0] state;
	localparam WAIT = 'b0001, 
					READ_UMQ1 = 'b0010,
				  READ_UMQ2 = 'b0011,
				  CHK = 'b0100,
				  READ_prev = 'b0101,
				  READ_current_2 = 'b0110,
				  READ_current = 'b0111,
				  WR_prev = 'b1000,
				  WR_current = 'b1001,
				  WR_FQ = 'b1010;
				  //WR_current = 'b1011,
				  //WR_FQ = 'b1100;
				  //FINDING = 'b1101;
				  
	assign match = (umq_data[`COMM] == {8'b0,message[111:104]}) && 
				(umq_data[`SRC] == {8'b0,message[103:96]})  && 
				(umq_data[`TAG] == {8'b0,message[95:88]});
	
	always @ (posedge clk) begin
		if (rst) begin
		  found <= 'b0;
		  posted_request <= 'b0;
		  not_found <= 'b0;
		  ptr <= UMQ_head;
		  prev <= `NULL;
		  req_r <= 'b0;
		  umq_data <= 'b0;
		  UMQ_tail <= `NULL;
		  freeQ_tail <= 16'b1111111111111111;
		  req_w <= 'b0;
		  current_data	<= 'b0;
		  prev_data <= 'b0;
		end else begin
				case (state) 
					WAIT: begin
						found <= 'b0;
						posted_request <= 'b0;
						not_found <= 'b0;
						req_r <= 'b0;
						umq_data <= 'b0;
						shift_UMQ_head	<= 'b0;
						req_w <= 'b0;		
						current_data	<= 'b0;
						prev_data <= 'b0;	
						if (find) begin
							if (count == 0 ) begin //&& (!drain)) begin
								found <= 'b0;
								not_found <= 'b1;
								posted_request <= 'b0;
								state <= WAIT;
							end else begin
								state <= READ_UMQ1;
								ptr <= UMQ_head;
								prev <= `NULL;
							end
						end
					end
					READ_UMQ1: begin
						req_w <= 'b0;
						req_r <= 'b1;
						req_addr <= ptr;					
						//prev <= ptr_prev;
						state <= READ_UMQ2;
						current_data	<= 'b0;
						prev_data <= 'b0;
					end
					//assuming reading takes one cycle
					READ_UMQ2: begin
						umq_data <= req_data_q;
						req_r <= 'b0;
						state <= CHK;
					end
					//check if it matches
					CHK: begin
						if (match) begin
							posted_request <= umq_data[`REQUEST];
							state <= READ_prev;						
						end
						//search next one
						else begin 
							if (umq_data[`NEXT_ADDR] != `NULL) begin
								ptr <= umq_data[`NEXT_ADDR];
								prev <= ptr;
								state <= READ_UMQ1;
							end else begin
								not_found <= 'b1;
								state <= WAIT;
							end
						end
					end
					READ_prev: begin
						//if it is the head of the UMQ, dont read or write prev,read write the ptr
						if (prev == `NULL) begin
							req_r <= 'b1;
							req_addr <= ptr;
							state <= READ_current_2;				
						end
						//if it is the end of the UMQ, change UMQ_tail
						else if(ptr == UMQ_tail) begin
							UMQ_tail <= prev;
							state <= READ_current;
						end
						else begin
							req_r <= 'b1;
							req_addr <= prev;
							state <= READ_current;
						end
					end
					READ_current_2: begin
						current_data <= req_data_q;
						//req_addr <= ptr;
						shift_UMQ_head <= 'b1;
						UMQ_newhead_req <= current_data[`NEXT_ADDR];
						state <= WR_current;
					end
					//assuming reading takes one cycle
					READ_current: begin
						prev_data <= req_data_q;
						req_addr <= ptr;
						state <= WR_prev;
					end
					//assuming reading takes one cycle
					WR_prev: begin
						current_data <= req_data_q;
						req_r <= 'b0;
						//prev points to next of current
						req_w <= 'b1;
						req_addr <= prev;
						req_data_d <= {current_data[`NEXT_ADDR], prev_data[239:0]};
						state <= WR_current;
					end
					//change the current q to point to null
					WR_current: begin
						req_w <= 'b0;
						shift_UMQ_head <= 'b0;
						req_addr <= ptr;
						req_data_d <= {`NULL, 240'b0};
						state <= WR_FQ;
					end
					WR_FQ: begin
						req_addr <= freeQ_tail;
						req_data_d <= {ptr, 240'b0};
						freeQ_tail <= ptr;
						state <= WAIT;
					end
					default: begin
						state <= IDLE;
					end							
				endcase
		end	
	end
	
 
//RAM queue(


//);
	
	
endmodule
	
	
	
	
	