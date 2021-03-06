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
module UMQ
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
	output Q_empty,
	output reg [packetizer_width-1:0] unexpected_message,
	input  clk, 
	input  rst
	);
	//'b0 address is NULL
	//reg [DATA_WIDTH-1:0] list[2**ADDR_WIDTH-1:0];
	
	reg [255:0] umq_data,current_data, prev_data, netdata_d, req_data_d;
	wire [255:0] netdata_q,req_data_q; 
	reg [127:0] message_r;
	reg [15:0] ptr, prev, net_addr,freeQ_next;
	reg req_r, req_w, net_w, net_r,shift_Q_head_net,  shift_UMQ_head;
	reg [3:0] NET_s;
	
	//unexpected queue head and tail ptrs
	reg [15:0] UMQ_head, UMQ_tail;
	reg [15:0] freeQ_head, freeQ_tail;
	reg [15:0] req_addr, UMQ_newhead_req, UMQ_newhead_net;
	//keep track of the queue condition
	integer count, i;
	
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
			if (found) begin
				count = count - 1;
			end
		end
	end
	
	//********************************FSM inserting when receive from network
		localparam IDLE = 'b0001, 
					READ1 = 'b0010,
					READ2_a = 'b0011,
				  READ2 = 'b0100,
				  INSERT1 = 'b0101;
				 // READ_UMQ2 = 'b0101,
				  //CHK = 'b0110,
				 // READ_prev = 'b0111,
				  //READ_current_2 = 'b1000,
	reg [3:0] state;			  
	always @ (posedge clk) begin
		if (rst) begin
			//UMQ_head <= 'b0;
			//UMQ_tail <= 'b0;
			freeQ_head <= 'b1;
			//freeQ_tail <= 16'b1111111111111111;
			net_w <= 'b0;
			freeQ_next <= `NULL;
			//net_r <= 'b0;
			net_addr <= `NULL;
			message_r <= 'b0;
			shift_Q_head_net <= 'b0;
		end else begin
			case (NET_s) 
			   IDLE: begin
					shift_Q_head_net <= 'b0;
					net_w <= 'b0;
					freeQ_next <= `NULL;
					//net_r <= 'b0;
					net_addr <= `NULL;
					if (insert) begin 
					   NET_s <= READ1;
						message_r <= message;
					end
				end
				//read the freeQ head next pointer,assuming it takes one cycle to read
				READ1: begin
				   net_addr <= freeQ_head;
					//net_r <= 'b1;	
					NET_s <= READ2_a;
				end
				READ2_a: begin 
					NET_s <= READ2;
				end
				READ2: begin 
					freeQ_next <= netdata_q[`NEXT_ADDR];
					NET_s <= INSERT1;
				end
				//write to the head of the freeQ and 
				INSERT1: begin
					netdata_d <= {UMQ_head,{8'b0,message_r[111:104]},{8'b0,message_r[103:96]}, {8'b0,message_r[95:88]}, 64'b0,message_r};
					net_w <= 'b1;
					NET_s <= IDLE;
					net_addr <= freeQ_head;
					freeQ_head <= freeQ_next;
					shift_Q_head_net <= 'b1;
					UMQ_newhead_net <= freeQ_head;
				end
				default: begin
					NET_s <= IDLE;
				end	
			endcase
		end
	
	end				
	
	always @ (posedge clk)	begin
		if (rst) begin
			UMQ_head <= `NULL;
		end else begin
			if (shift_UMQ_head) begin
				UMQ_head <=  UMQ_newhead_req;			
			end else if (shift_Q_head_net) begin
				UMQ_head <= UMQ_newhead_net;			
			end 
		end	
	end

	/**************FSM for data searching***********************/
	wire match;
	
	
	localparam WAIT = 'b0001, 
					READ_UMQ1 = 'b0010,
				  READ_UMQ2 = 'b0011,
				  READ_UMQ3 = 'b0100,
				  CHK = 'b0101,
				  READ_prev = 'b0110,
				  READ_current_2_a = 'b0111,
				  READ_current_2 = 'b1000,
				  READ_current_a = 'b1001,
				  READ_current = 'b1010,
				  WR_prev_a = 'b1011,
				  WR_prev = 'b1100,
				  WR_current = 'b1101,
				  WR_FQ = 'b1110,
				  WR_prev_b = 'b1111;
				  
				  //WR_FQ = 'b1100;
				  //FINDING = 'b1101;
				  
	assign match = (umq_data[`COMM] == {8'b0,request[23:16]}) && 
				(umq_data[`SRC] == {8'b0,request[15:8]})  && 
				(umq_data[`TAG] == {8'b0,request[7:0]});

	
	
	always @ (posedge clk) begin
		if (rst) begin
		  found <= 'b0;
		  unexpected_message <= 'b0;
		  not_found <= 'b0;
		  ptr <= UMQ_head;
		  prev <= `NULL;
		  //req_r <= 'b0;
		  umq_data <= 'b0;
		  UMQ_tail <= 'b1;
		  freeQ_tail <= 16'b0000001111111111;
		  req_w <= 'b0;
		  current_data	<= 'b0;
		  prev_data <= 'b0;
		  shift_UMQ_head	<= 'b0;
		end else begin
				case (state) 
					WAIT: begin
						found <= 'b0;
						unexpected_message <= 'b0;
						not_found <= 'b0;
						//req_r <= 'b0;
						umq_data <= 'b0;
						shift_UMQ_head	<= 'b0;
						req_w <= 'b0;		
						current_data	<= 'b0;
						prev_data <= 'b0;	
						if (find) begin
							if (count == 0 ) begin //&& (!drain)) begin
								found <= 'b0;
								not_found <= 'b1;
								unexpected_message <= 'b0;
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
						//req_r <= 'b1;
						req_addr <= ptr;					
						//prev <= ptr_prev;
						state <= READ_UMQ2;
						current_data	<= 'b0;
						prev_data <= 'b0;						
					end
					//assuming reading takes two cycle
					READ_UMQ2: begin
						state <= READ_UMQ3;
					end
					READ_UMQ3: begin
						umq_data <= req_data_q;
						//req_r <= 'b0;
						state <= CHK;
					end
					//check if it matches
					CHK: begin
					   umq_data <= req_data_q;
						if (match) begin
						   unexpected_message <= umq_data[`MESSAGE];
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
							//req_r <= 'b1;
							req_addr <= ptr;
							state <= READ_current_2_a;				
						end
						//if it is the end of the UMQ, change UMQ_tail
						else if(ptr == UMQ_tail) begin
							UMQ_tail <= prev;
							req_addr <= prev;
							state <= READ_current_a;
						end
						else begin
							//req_r <= 'b1;
							req_addr <= prev;
							state <= READ_current_a;
						end
					end
					READ_current_2_a: begin
						state <= READ_current_2;						
					end
					READ_current_2: begin
						current_data <= req_data_q;
						//req_addr <= ptr;
						shift_UMQ_head <= 'b1;
						UMQ_newhead_req <= current_data[`NEXT_ADDR];
						state <= WR_current;
					end
					READ_current_a: begin
						state <= READ_current;	
					end
					//assuming reading takes two cycle
					READ_current: begin
						prev_data <= req_data_q;
						req_addr <= ptr;
						state <= WR_prev_a;
					end
					//assuming reading takes two cycle
					WR_prev_a: begin
						state <= WR_prev_b;
					end
					WR_prev_b: begin
						current_data <= req_data_q;
						state <= WR_prev;
				   end
					WR_prev: begin
						current_data <= req_data_q;
						//req_r <= 'b0;
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
						req_w <= 'b1;
						req_data_d <= {ptr, 240'b0};
						freeQ_tail <= ptr;
						state <= WAIT;
						found <= 'b1;
					end
					default: begin
						state <= IDLE;
					end							
				endcase
		end	
	end
	
 

	ram umq_queue(
	.data_a(netdata_d), 
	.data_b(req_data_d),
	.address_a(net_addr), 
	.address_b(req_addr),
	.wren_a(net_w),
	//.re_n(net_r),
	.wren_b(req_w),
	//.re_req(req_r), 	
	.clock(clk), 
	//.rst(rst),
	.q_a(netdata_q),
	. q_b(req_data_q)
	);
endmodule
	
	
	
	
	