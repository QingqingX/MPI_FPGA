/**************************************************
Using double linked-list as PRQ and UMQ.
How the Qs work? 
1. When one packet comes from network, goes to PRQ to find?
	1.y store data to the address and increase complete register by one,
	when wait all pulls on complete signal(cr == wait number)
	1.n store packets to UMQ, 
2. when one request from NIOS ii, goes to UMQ to find?
	2.y store data to address adn increase complete register number
	3.n stores to PRQ.
	4.when wait, execute PRQ one by one
*/


/********************************************
1. scheme i: whenever there is packet from network
				 request will find the packet, if not imediately, store to PRQ
				 when wait, execute PRQ one by one
				 
				


********************************************/

`timescale 1 ps / 1 ps
module acc_recv #(
		parameter cpu_width = 32,
		parameter packetizer_width = 128,
		parameter data_width = 32,
		parameter CONTENT_WIDTH =36,
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
	   input wire packet_in_valid,	
		/////////////////these goes to the main memory//////////////////////////
		output wire        write,
		output wire [31:0] write_addr,
		output wire [data_width-1:0]   data_to_mem  
		 //same as chipselect here, only for interfacing as avalon memory mapped master
	);
	
	reg   [SIZE-1:0]          state, finder_state        ;
	reg   [10:0]				  tag, data_size;
	//wire  [10:0]				  tag_w;
	//reg 	[data_width-1:0] 	  write_addr_r;//these goes to mem ctrl
	reg        					  done_r;
	wire [data_width-1:0]	  data, posted_request;
	wire PRQ_empty,UMQ_empty, not_found_PRQ,found_PRQ;
	reg insert_PRQ, finding_PRQ;
	localparam WAIT = 3'b000,
	     
				  RNDV = 3'b001,		//long message,generate and send CTS message		  				   
				  MEM = 3'b010,	//start writing to mem and announce CPU
				  FIN = 3'b011,
				  LOOKING = 3'b101;
	localparam IDLE = 3'b000,
				  CHECK = 3'b001,
				  DRAIN = 3'b010,
				  FINDING = 3'b011,		//long message,generate and send CTS message		  				   
				  WAIT_FOR_DATA = 3'b100,	//start writing to mem and announce CPU
				  FOUND = 3'b101;			

	wire [255:0] ignore;
	reg insert_EMQ;
	wire not_found;
	
	
	//***********EAGER
	reg eager,write_mem_valid;
	//***********request
   reg req;	
	//***********DATA
	reg DATA;
	//***********result
	reg [data_width-1:0] result_r;
	
	//reg end_write;

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
	reg complete_1,complete_2,clean_number;
	reg [31:0] complete_number;
	always @ ( posedge nios_clk)
	begin
		if(reset || clean_number) begin
			complete_number <= 'b0;
		end else begin
			if (complete_1) 
				complete_number <= complete_number + 1;
			if (complete_2)
				complete_number <= complete_number + 1;
		end
	end
	
	
	
	//*************data finder FSM, whenever there is request from processor*************************************/
	//*************triggers this FSM, and find UMQ, if not found after one iteration, 
	//*************stores data in PRQ
	reg finding;
	wire found;
	wire [31:0] data_from_short_cam;
	reg [31:0] request, data_ptr; 
	
	integer timer;
	reg waitall;
	always @ (posedge nios_clk) begin
		if (reset == 1'b1 || clean_number)
			waitall = 'b0;
		else	
			waitall = (data_in_a[31:24] == 8'b11111111);
	end
	
		integer cont;
	reg start_r;
	//small FSM
	localparam ZERO=2'b00,
					ONE = 2'b01,
					TWO = 2'b10;
					
	reg [1:0] start_state;
	always @ (posedge nios_clk)
	if (reset) begin
	   cont <= 0;
		start_r <= 0;
	end else begin 
		case (start_state) 
			ZERO: begin
				if (start) begin
					start_r <= 1;
					cont <= cont + 1;
					start_state <= ONE;
				end
			end
			ONE: begin
				if (cont == 1) 
					start_state<= TWO;
				else cont <= cont + 1;
			end
		   TWO: begin
				cont <= 0;
				start_r <= 0;
				start_state <= ZERO;
			end
			default: begin
				start_state <= ZERO;
			end
		endcase
	end
	
	always @ (posedge nios_clk)
	begin : DATA_FINDER
	   if (reset == 1'b1) begin
			result_r <= 'b0;
		   done_r <= 1'b0;
			finding <= 1'b0;
			insert_PRQ <= 'b0;
			timer <= 0;
			clean_number <= 1'b1;
			data_size <=  'b0;
		end else begin
			case (finder_state)
				IDLE: begin
				   complete_1 <= 'b0;
					insert_PRQ <= 'b0;
					done_r <= 1'b0;
					clean_number <= 1'b0;
					if ( start_r ) begin
						request <= data_in_a;
						data_ptr <= data_in_b;
						insert_PRQ <= 'b0;
						data_size <= data_in_b[10:0];
						if (waitall) begin	//blocking waitall
							//drain PRQ
							timer <= 0;
							finder_state <= DRAIN;
							
						end
						else begin					
							finding <= 1'b1;
							finder_state <= FINDING;
						end						
					end
				end
				/*
				CHECK: begin
					if (waitall) begin	//blocking waitall
						//drain PRQ
						timer <= 0;
						finder_state <= DRAIN;
						
					end
					else begin					
							finding <= 1'b1;
							finder_state <= FINDING;
					end
				end
				*/
				DRAIN: begin
					timer <= timer + 1;
					if (data_size == complete_number) begin
							result_r <= 32'h52052020;
						   done_r <= 1'b1;
							finder_state <= IDLE;
							clean_number <= 1'b1;
					end else if (timer > 100000000) begin //100mhz, 100 million cycles, 1 second
							result_r <= 32'hdeaddead;    //receive this, means time out 
						   done_r <= 1'b1;
							finder_state <= IDLE;
							clean_number <= 1'b1;
					end
				end
				FINDING: begin
				   //search UMQ for eager message  
					finding <= 1'b0;
					if (found != 'b0) begin //find data
						//eager message found
						
						finder_state <= FOUND;
						result_r <= data_from_short_cam[31:0];
							  done_r <= 1'b1;
						complete_1 <= 'b1;

				   end
					//else, if not_found
					if (not_found == 'b1 || UMQ_empty) begin
						insert_PRQ <= 'b1;
						finder_state <= FOUND;
						result_r <= 32'haaaadead;    //not found this time, stores in PRQ
						done_r <= 1'b1;
					end
				end 
				FOUND: begin
					insert_PRQ <= 'b0;
					finding <= 1'b0;
				   done_r <= 1'b0;
					finder_state <= IDLE; 
					timer <= 0;
					complete_1 <= 'b0;
				end
				default: begin
					finding <= 1'b0;
				   done_r <= 1'b0;
					finder_state <=IDLE;
				end
		   endcase
		end
	
	end 
	
	//***************data finder connecting to UMQ
		wire [15:0] req_match;
	assign req_match = packet_in[103:88];
	
	cam_prq #(.packetizer_width(128), 
			.DATA_WIDTH(256), 
			.CONTENT_WIDTH(CONTENT_WIDTH),
			.ADDR_WIDTH(16),
			.COMM_COUNT(1),
			.RANK_BIT(8),
			.TAG_BIT(8))
   PRQ0(
	.request(request),    //from read request
	.data_ptr(data_ptr),
	.message(req_match),//data_from_router),  //input from network
	.find(finding_PRQ),
	.insert(insert_PRQ),
	.found(found_PRQ), 
	.not_found(not_found_PRQ),
	.Q_empty(PRQ_empty),
	.posted_request(posted_request),
	.clk(nios_clk), 
	.rst(reset)
	);
	
	cam_wildcard #(.packetizer_width(128), 
.DATA_WIDTH(256), 
.CONTENT_WIDTH(CONTENT_WIDTH),
.ADDR_WIDTH(16),
.COMM_COUNT(1),
.RANK_BIT(8),
.TAG_BIT(8))
	UMQ(
	.request(request),    //from read request
	.message(data_from_router),  //input from network
	.find(finding),
	.insert(insert_EMQ),
	.found(found), 
	.not_found(not_found),
	.Q_empty(UMQ_empty),
	.unexpected_message(data_from_short_cam),
	.clk(nios_clk), 
	.rst(reset)
	);
	
	
	
	
	/********************************network message centric receive finder********************************************************/
	/****************************************************FSM***********************************************************************/
	/**************when receives an eager message from network,
	//1.find PRQ, if not find goes to UMQ.  
	* if yes, modify the user memory //!!!!!now doing nothing, only increment the counter
	*/
	
   //*******************************request queue***************************************
	

	
	
	(*preserve*) reg [31:0] found_posted_request;
	always @ (posedge nios_clk)
	begin : OUT_PUT
		if (reset == 1'b1) begin
			//size = 11'b0;
			write_mem_valid <= 1'b0;
		end else begin
			case (state)
				WAIT: begin
				   write_mem_valid <= 1'b0;
					eager <= ((packet_in[127:123] == 5'b10000) && packet_in_valid) ? 1'b1 : 1'b0; //eager and long data packets have the same header 5-bit, inthis step, you cannot see long data
					data_from_router <= packet_in;
					//req_match <= packet_in[103:88];
					//size <= data_in_b[10:0];
					complete_2 <= 'b0;
					
					// add for unexpected eager messages
					if (eager) begin
						finding_PRQ <= 1'b1;
						state <= LOOKING;    //look into PRQ
					end	
				end
				LOOKING: begin
				   //search PRQ for eager message  
					finding_PRQ <= 1'b0;
					//req_match <= req_match;
					if (found_PRQ != 'b0) begin //find data
						state <= FIN;
						complete_2 <= 'b1;	
						found_posted_request <= posted_request;
						write_mem_valid <= 'b1;
				   end
					//else, if not_found
					if (not_found_PRQ == 'b1 || PRQ_empty) begin
						insert_EMQ <= 'b1;
						state <= FIN;
					end
				end 
				FIN: begin
					insert_EMQ <= 1'b0;
					state <= WAIT;
					complete_2 <= 'b0;
					write_mem_valid <= 'b0;
				end
				default: begin 
					state <= WAIT;
				end
			endcase 
		end 
	end	

	//write to mem
	
/*	
always @ (posedge nios_clk)
	if (reset)
		end_write = 0;
	else if (start && (packet_in[10:0] == size - 1))
		end_write = 1;
*/
		
assign write = write_mem_valid;
assign data_to_mem = data_from_router[87:56];
assign write_addr = found_posted_request;
assign done = done_r;
assign result = result_r;
endmodule