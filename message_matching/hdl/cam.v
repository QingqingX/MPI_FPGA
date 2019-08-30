/* return only the first match when it is wildcard*/

module cam
#(
parameter packetizer_width=128, 
parameter DATA_WIDTH=256, 
parameter CONTENT_WIDTH =48,
parameter ADDR_WIDTH=16,
parameter COMM_COUNT =1,
parameter RANK_BIT= 8,
parameter TAG_BIT= 8
) 
(
   input [31:0] request,    //from read request
	input [packetizer_width-1 : 0]  message,  //input from network
	input find,
	input insert,
	output reg found, 
	output reg not_found,
	output reg Q_full,
	output Q_empty,
	output reg [47:0] unexpected_message,   //only bits [103:56] are required
	input  clk, 
	input  rst
	);

	parameter NO_DATA=1'b0;
	parameter VALID_DATA=1'b1;

	integer count;
	reg insert_fin, req_w, net_w;
	reg [ADDR_WIDTH-1:0] net_addr, req_addr;
	wire [CONTENT_WIDTH-1:0] netdata_q,req_data_q;
   reg [CONTENT_WIDTH-1:0] netdata_d;
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
	
	//********mask to the content of the cam
	reg [ADDR_WIDTH-1 :0] mbits;
	wire [ADDR_WIDTH-1 :0] pattern;

	
	always @ (posedge clk) begin
		if (rst) begin
			mbits <= 'b0;
		end 
		else begin
			if (found) begin
				mbits[req_addr] <= 'b0;
			end 
			if (insert_fin) begin
				mbits[net_addr] <= 'b1;
			end
		end
		
	end
	
	//*************************************find the first match*****************************************/
	reg [3:0] find_state;
	localparam WAIT = 'b0001, 
					READ_UMQ1 = 'b0010,
				  READ_UMQ2 = 'b0011;
				  
	always @ (posedge clk) begin
		if (rst) begin
			found <= 'b0;
			not_found <= 'b0;
		end 
		else begin
			case (find_state) 
				WAIT: begin
					found <= 'b0;
					unexpected_message <= 'b0;
					not_found <= 'b0;
					if (find) begin
						if (count == 0 ) begin 
							found <= 'b0;
							not_found <= 'b1;
							unexpected_message <= 'b0;
							find_state <= WAIT;
						end else begin							
							req_addr <= request[15:0];   //the address
							if (mbits[request[15:0]] == 'b0) begin//no data
								found <= 'b0;
								not_found <= 'b1;
								find_state <= WAIT;
							end
							else begin //find
								find_state <= READ_UMQ1;
							end
						end
					end
				end
				READ_UMQ1: begin
					find_state <= READ_UMQ2;				
				end
				//assuming reading takes two cycle
				READ_UMQ2: begin
					unexpected_message <= req_data_q;   //48 bits data came
					found <= 'b1;
					not_found <= 'b0;
					find_state <= WAIT;
				end
				default: 
					find_state <= WAIT;
			endcase
		end
	end
	
	//************************************insert from network**********************************************************
	reg [3:0] store_state;
	reg [127:0] message_r;
	localparam IDLE = 'b0001, 
					FIN = 'b0010;
				  
	assign pattern = message[103:88]; // the header: src rank, tag, 24'b, only 1 communicator, 16'b,32k*6B, 
	always @ (posedge clk) begin
		if (rst) begin
			net_w <= 'b0;
			insert_fin <= 'b0;
		end 
		else begin
			case (store_state) 
				IDLE: begin
					net_w <= 'b0;
					net_addr <= 'b0;
					insert_fin <= 'b0;
					if (insert) begin
						netdata_d <= message[103:56];
						net_addr <= pattern;
						net_w <= 'b1;
						store_state <= FIN;
					end
				end
				FIN: begin //two cycles for writing a data
				   insert_fin <= 'b1;
					store_state <= IDLE;				
				end
				default: 
					store_state <= IDLE;
			endcase
		end
	end
	
	ram_dp #(
	.DATA_WIDTH(CONTENT_WIDTH),
	.ADDR_WIDTH(ADDR_WIDTH)
	) 
	umq_queue(
	//network write to this cam
	.wren_a(net_w),
	.data_a(netdata_d), 	
	.address_a(net_addr), 
	
	//request side read the data
	.data_b('b0),
	.address_b(req_addr),	
	.wren_b('b0),	
	.clock(clk), 
	.q_a(netdata_q),
	. q_b(req_data_q)
	);
endmodule
