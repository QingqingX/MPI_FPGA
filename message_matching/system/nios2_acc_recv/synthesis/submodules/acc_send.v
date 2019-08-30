`timescale 1 ps / 1 ps
/*
data_a = 32'b01010101_01010101_00000000_00000001;   /type, source, dst of the data ,e.g. 0 to 1
	data_b = 32'b00000000_00_00000000100_00000000001; // when it is eager short, data is stored as highest 10 bit
	in = 3'b111;
    
*/
module acc_send #(
		parameter cpu_width = 32,
		parameter packetizer_width = 128,
		parameter data_width = 32,
		parameter mem_width = 32,
		parameter mem_depth = 11,
		parameter threshold  = 1,
		parameter SIZE = 3
	) (
		input  wire        nios_clk,   // in_clk_0.clk
		input  wire 		 clk_en,
		input  wire        reset, // in_rst_0.reset
		input  wire [31:0] data_in_a,      //     header
		input  wire [31:0] data_in_b,      //     data or pointer to data
		input  wire [2:0]  in_opcode,     //    mpi type  
		input  wire 		 start,
		output wire [31:0] result,
		output wire 		 done,   
		/////////////////these goes to the router///////////////////////////////
		output reg [packetizer_width-1:0] packet,
	   output     packet_out_valid,	
	   //input wire 	[packetizer_width-1:0] packet_in
		/////////////////these goes to the main memory//////////////////////////
		
		output wire [19:0] mem_addr,
		input  wire [data_width-1:0]   data_from_mem,   
		output wire read //same as chipselect here, only for interfacing as avalon memory mapped master
		
		/////////////////these comes from router///////////////////////////////
		//input wire hold
		
	);
	
	reg   [SIZE-1:0]          state, next_state        ;
	reg   [10:0]				  size, tag;
	reg   [data_width-1:0]    head, pointer;
	wire  [10:0]				  tag_w;
	reg 	[data_width-1:0] 	  read_addr;//these goes to mem ctrl
	reg        					  read_mem_valid, done_r, done_r_2;
	wire [data_width-1:0]	  data;
	//assign mem_clk = nios_clk;
	//assign mem_reset = reset;
	localparam WAIT = 3'b000,
				  //STORE = 3'b001,
				  CHECK = 3'b010,				  				  
				  RNDV = 3'b011,   //long message, generate and send RTS to dst node, and wait for CTS message
				  MEM = 3'b100,	//when receive CTS, jump from RNDV to this one
				  SEND_Eager = 3'b101,
					SEND = 3'b111;
	//state machine 
	always @ (posedge nios_clk)
	begin : FSM_REQ
		if (reset == 1'b1) begin
			done_r_2 <= 1'b0;
		end else begin
			done_r_2 = done_r;
		end 
	end
	//***********CTS
	//reg   CTS;
	//reg [26:0] cts_new_info;
	//***********opcode
	reg [2:0] opcode;
	reg [31:0] result_r;
	/*
	always @ (posedge nios_clk)
	begin : FSM_CTS
		if (reset == 1'b1 || done == 1'b0) begin
			CTS <= 0;
			cts_new_info <= 27'b0;
		end else if (packet_in[127:123] == 5'b01110) begin  //CTS coming 
			cts_new_info <= packet_in[103:77];
			CTS <= 1;
		end 
	end
	*/
/*	
	always @ (state or start or tag or size or CTS)
	begin : STATE_DIAGRAM
		case (state)
			WAIT: begin
			   done_r = 1'b0;
				if (clk_end) begin	
					if (start) begin 
						if (size > threshold) 
							state = RNDV;
						else 
							state = SEND_Eager;
					end
				end
			end
			SEND_Eager: begin					
				   state = WAIT;
					done_r = 1'b1;
			end 
			RNDV: begin
				done_r = 1'b0;
		      if (CTS)
					state = MEM;
			end
			MEM: begin
				state = SEND;
			end
			SEND: begin
				if (tag == size-1) begin
					state = WAIT;
					done_r = 1'b1;
				end
			end
			default: begin
				state = WAIT;
			end
		endcase
	end
	*/
	reg [10:0] counter;
	reg data_valid;
	always @ (posedge nios_clk)
	begin : OUT_PUT
		if (reset == 1'b1) begin
			head = 32'b0;
			pointer = 32'b0;
			size = 11'b0;
			packet = 128'b0;
			counter = 'b0;
			read_mem_valid = 1'b0;
			read_addr = 32'b0;
			result_r = 32'b0;
			done_r = 1'b0;
			data_valid = 1'b0;
			
		end else begin
			case (state)
				WAIT: begin
					if (clk_en) begin
					data_valid = 1'b0;
					done_r = 1'b0;
						if (start) begin
							head = data_in_a;		//store the type, source, dst of the data 
							pointer = data_in_b; //store pointer to the data in memory/ when it is eager, this is the actual data
							size = data_in_b[10:0];
							base_addr = data_in_b[31:11]; //added for datatype paper
							opcode = in_opcode;
							if (in_opcode == 3'b010)  begin //eager
								state = SEND_Eager;
							end
							else if (in_opcode == 3'b011)	begin
								read_addr = data_in_b;
								read_mem_valid = 1'b1;
								state = SEND;
							end
							/*********memory part******** read memory*/
							counter = 'b0;	
					   end
					end
				end
				
				SEND_Eager: begin
					packet = {5'b10000, opcode, head, pointer, 45'b0, 11'b0}; //data packet 5'b, 3'b, 32, 32, 45 ,11
					data_valid = 1'b1;
					result_r = pointer;
					
					state = WAIT;	
					done_r = 1'b1;
				end			
				/*
				RNDV: begin //generate RTS
					packet = {5'b10001, opcode, head, size, 77'b0};  //RTS packet starts with 5'b10001,3'bopcode,32'bhead,size, 77'b0;
					//if (CTS)
					data_valid = 1'b1;
					state = MEM;
				end
				*/
				/*
				MEM: begin
				   data_valid = 1'b0;
					read_addr = pointer;
					read_mem_valid = 1'b1;	
					
				end */
				SEND: begin
					if (counter == 0) begin //header
						packet = {5'b10001, opcode, head, size, pointer, 24'b0}; //data packet 5'b, 3'b, 32, 32, 45 ,11
						data_valid = 1'b1;
						counter = counter + 'b1;
						read_addr = read_addr+'b1;
					end
					else if (counter == size) begin
						state = WAIT;
						done_r = 1'b1;
						read_mem_valid = 1'b0;
						packet = {5'b11001, opcode, head, size, data_from_mem, 24'b0}; //data packet 5'b, 3'b, 32, 32, 45 ,11
						data_valid = 1'b1;
						
					end else begin
						result_r = data;
						data_valid = 1'b1;
						packet = {5'b11000, opcode, head, size, data_from_mem, 24'b0}; //data packet 5'b, 3'b, 32, 32, 45 ,11
						counter = counter + 'b1;					
						read_addr = read_addr+'b1;
					end 
				end
				
				default: begin
					data_valid = 1'b0;

					head = 32'b0;
					pointer = 32'b0;
					size = 11'b0;
					packet = 128'b0;
					done_r = 1'b0;

					/*********memory part********/
					counter = 'b0;
					read_mem_valid = 1'b0;
					read_addr = 32'b0;
				end
			endcase 
		end 
	end
assign mem_addr = read_addr;
assign done = done_r;
assign result = result_r;
assign packet_out_valid = data_valid;
assign read = read_mem_valid;
/*
mem mem_main(nios_clk, reset,hold, read_addr, read_mem_valid&&(~hold), data, tag_w, 
				read, mem_addr, data_from_mem); //THIS LINE needs to go all the way to the top level of the accelerator
*/	
endmodule
		
	