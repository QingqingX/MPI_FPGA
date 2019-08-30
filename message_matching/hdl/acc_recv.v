`timescale 1 ps / 1 ps
module acc_recv #(
		parameter cpu_width = 32,
		parameter packetizer_width = 128,
		parameter data_width = 32,
		parameter mem_width = 32,
		parameter mem_depth = 11,
		parameter threshold  = 1,
		parameter SIZE = 3
	) (
		input wire        nios_clk,   // in_clk_0.clk
		input wire        clk_en,
		input wire        reset, // in_rst_0.reset
		input wire [31:0] data_in_a,      //     header
		input wire [31:0] data_in_b,      //     data or pointer to data
		input wire [2:0]  in_opcode,     //    mpi type  
		input wire 		 start,
		output wire [31:0] result,
		output wire 		 done, 		
		/////////////////these comes from the router///////////////////////////////
		input wire [packetizer_width-1:0] packet_in,  
		output reg [packetizer_width-1:0] packet_out,
		/////////////////these goes to the main memory//////////////////////////
		output wire        write,
		output wire [10:0] write_addr,
		output wire [data_width-1:0]   data_to_mem  
		 //same as chipselect here, only for interfacing as avalon memory mapped master
	);
	
	reg   [SIZE-1:0]          state, finder_state        ;
	reg   [10:0]				  size, tag, data_size;
	reg   [data_width-1:0]    head, pointer;
	//wire  [10:0]				  tag_w;
	//reg 	[data_width-1:0] 	  write_addr_r;//these goes to mem ctrl
	reg        					  done_r;
	wire [data_width-1:0]	  data;
	
	localparam WAIT = 3'b000,
				  RNDV = 3'b001,		//long message,generate and send CTS message		  				   
				  MEM = 3'b010,	//start writing to mem and announce CPU
				  FIN = 3'b011;
				  //MEM2 = 3'b101,
	localparam IDLE = 3'b000,
				  FINDING = 3'b001,		//long message,generate and send CTS message		  				   
				  WAIT_FOR_DATA = 3'b010,	//start writing to mem and announce CPU
				  FOUND = 3'b011;			
	//state machine 

	//***********RTS
	reg   RTS;
	reg [26:0] rts_new_info, req_RNDV;
	
	always @ (posedge nios_clk)
	begin : FSM_REQ
		if (reset == 1'b1 || done == 1'b1) begin
			RTS <= 0;
			rts_new_info <= 27'b0;
		end else if (packet_in[127:123] == 5'b10001) begin  //RTS coming 
			rts_new_info <= packet_in[103:77];
			RTS <= 1;
		end 
	end
	//***********EAGER
	reg eager;
	//***********request
   reg req;	
	//***********DATA
	reg DATA;
	//***********result
	reg [data_width-1:0] result_r;
	
	reg write_mem_valid, end_write;

	reg [packetizer_width-1:0] data_from_router;
	/*************************************************new implementation*****************************************/
	/************************************Request centric receving engine*****************************************/
	//Whatever(RNDV or EAGER) the received packet is, store in the data table
	//if RDVN, stores and waits at RNDV state for data_finder
	//when data finder comes and find the RNDV from the table, send CTS and waits for data at mem
	//when data comes, return    (!!!RNDV needs more thinking)
	//1)if EAGER, stores and waits at WAIT state for other data 
	//when data finder comes, it searches the corresponding entry for the data,and mark it as used then return the data to cpu
	//2)if data_finder of a eager receive comes to find data, but not found, it will keep waiting for the data
   //Only cares about the request 	
	
	
	//*************data finder
	reg [1:0] src_info, dst_info;
	reg finding;
	wire [packetizer_width-1:0] data_from_short_cam;
	
	
	always @ (posedge nios_clk)
	begin : DATA_FINDER
	   if (reset == 1'b1) begin
			result_r <= 'b0;
		   done_r <= 1'b0;
			finding <= 1'b0;
		end else begin
			case (finder_state)
				IDLE: begin
					if ( start) begin
						src_info <= data_in_a[9:8];    //only use two bits of the 8
						dst_info <= data_in_a[1:0];
						data_size <= data_in_b[10:0];
						finder_state <= FINDING;
					end
				end
				FINDING: begin
				   //search for eager message  
					finding <= 1'b1;
					if (data_from_short_cam != 'b0) begin //find data
					   //RNDV message found
						if (data_from_short_cam[packetizer_width-1:packetizer_width-5] == 5'b10001) begin
							finder_state <= WAIT_FOR_DATA;
							req_RNDV <= data_from_short_cam[103:77]; 
						end
						//eager message found
						else begin
							finder_state <= FOUND;
							result_r <= data_from_short_cam[87:56];
						   done_r <= 1'b1;
						end
				   end
					//else, keep finding
				end 
				//RNDV data, wait for receiving engine to send CTS and wait for data
				WAIT_FOR_DATA: begin
					if(DATA) begin
						result_r <= packet_in[87:56];
						done_r <= 1'b1;
						finder_state <= FOUND;
					end
				end
				FOUND: begin
					finding <= 1'b0;
				   done_r <= 1'b0;
					finder_state <= IDLE; 
				end
				default: begin
					finding <= 1'b0;
				   done_r <= 1'b0;
					finder_state <=IDLE;
				end
		   endcase
		end
	
	end 
	
	//***************data finder connecting to CAM
	wire [255:0] ignore;
	reg we_RTS_cam, we_eager_cam;
	CAM short_data_cam(.data_a(data_from_router),    //write from network to cam
							 .data_b('b0),   		//nothing to write 
							 .addr_a(head[9:8]),			//write address, src0: 00, src1: 01;src2:10, src3:11, can be more srces
							 .addr_b(src_info),       //read address
							 .we_a(we_eager_cam),			//write request by network 
							 .re_b(finding), 			//read request by data finder
							 .clk(nios_clk), 
							 .rst(reset),
							 .q_a(ignore),    //ignore
							 .q_b(data_from_short_cam)
							 );
							 
	
	
	
	
	always @ (posedge nios_clk)
	begin : OUT_PUT
		if (reset == 1'b1) begin
			head = 32'b0;
			pointer = 32'b0;
			size = 11'b0;
			write_mem_valid = 1'b0;
		end else begin
			case (state)
				WAIT: begin
				   write_mem_valid <= 1'b0;
					eager <= (packet_in[127:123] == 5'b10000) ? 1'b1 : 1'b0; //eager and long data packets have the same header 5-bit, inthis step, you cannot see long data
					data_from_router <= packet_in;					
					head <= data_in_a;
					pointer <= data_in_b;
					size <= data_in_b[10:0];
					if (RTS) begin
					   we_RTS_cam <= 1'b1;
					   //push to data_table
						state <= RNDV;		
			      end
					// add for unexpected eager messages
				   // 1) when only eager message arrive unexpectly 
				   // store to the CAM-data_table, and go to the next wait cycle,
					// 2) if RNDV arrives later, then store RNDV and goes to RNDV state 
				   // then send CTS when there is a corresponding request	
					else if (eager) begin
					   we_eager_cam <= 1'b1;
						state <= FIN;
					end	
				end	
				RNDV: begin //generate CTS
				   if (req_RNDV == rts_new_info) begin   
						packet_out <= {5'b01110, in_opcode, head, size, 77'b0};  //CTS packet starts with 5'b01110,3'bopcode,32'bhead,88'b0;
					   state <= MEM;
					end 	
				end
				MEM: begin
				   DATA <= (packet_in[127:123] == 5'b11000) ? 1'b1:1'b0;
					if (DATA) begin
						write_mem_valid <= 1'b1;					
					end 
					if (end_write)
						state <= FIN;
				end
				FIN: begin
					we_eager_cam <= 1'b0;
				   we_RTS_cam <= 1'b0;
					state <= WAIT;
				end
				default: begin
					head <= 32'b0;
					pointer <= 32'b0;
					size <= 11'b0;
				   write_mem_valid <= 1'b0;
					state <= WAIT;
				end
			endcase 
		end 
	end	

always @ (posedge nios_clk)
	if (reset)
		end_write = 0;
	else if (start && (packet_in[10:0] == size - 1))
		end_write = 1;

		
assign write = write_mem_valid;
assign data_to_mem = packet_in[87:56];
assign write_addr = pointer[21:11] + packet_in[10:0];
assign done = done_r;
assign result = result_r;
endmodule