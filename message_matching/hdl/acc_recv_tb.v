`timescale 1 ps / 1 ps
module acc_recv_tb;


reg clk, reset, start, CTS, clk_en, valid;
reg [31:0] data_from_mem, data_a, data_b;
reg [2:0]  in;
reg [127:0] packet_in;
wire [127:0] packet_out;
wire [10:0] write_addr;
wire [31:0] data_to_mem;
wire write, done;
wire [31:0] result;


acc_recv acc_1(
                .nios_clk(clk),   // in_clk_0.clk
					 .clk_en(1'b1),
                .reset(reset), // in_rst_0.reset
                .data_in_a(data_a),      //     header
                .data_in_b(data_b),      //     data or pointer to data
                .in_opcode(in),     //    mpi type  
                .start(start),
                .result(result),
                .done(done),
                /////////////////these goes to the router///////////////////////////////
                .packet_in(packet_in),  
					 .packet_in_valid(valid),	
					 .packet_out(packet_out),
                /////////////////these goes to the main memory//////////////////////////
					 
 
					 .write(write),
		          .write_addr(write_addr),
		          .data_to_mem(data_to_mem)
        );
    
/*
integer j;
//reg [15:0] j_reg;
initial begin
  for(j = 1; j < 1023; j = j+1)  begin
    acc_1.UMQ.umq_queue.ram[j] = {j+'b1, 240'b0};
	 acc_1.PRQ0.prq_queue.ram[j] = {j+'b1, 240'b0}; 
	end
	acc_1.UMQ.umq_queue.ram[0] = 'b0;
	acc_1.UMQ.umq_queue.ram[1023] = 'b0;
	acc_1.PRQ0.prq_queue.ram[0] = 'b0;
	acc_1.PRQ0.prq_queue.ram[1023] = 'b0;
end
*/
parameter MEM_INIT_FILE = "ram.mif";

initial begin
  if (MEM_INIT_FILE != "") begin
    $readmemh(MEM_INIT_FILE, acc_1.UMQ.umq_queue);
	 $readmemh(MEM_INIT_FILE, acc_1.PRQ0.prq_queue);
  end
end
always #1 clk = ~clk;
initial begin

/** send, recv, send, recv, send, recv
        clk = 0;
        reset = 1;
        data_a = 32'b0;
        data_b = 32'b0;
        in = 3'b0;
        start = 0;
        data_from_mem = 32'b0;
		  valid = 'b0;
        #5 reset = 0;
		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00000000_00000010,32'h55555555,45'b0,11'b00000000000}; //data 2
		  valid = 'b1;
		  #2 valid = 'b0;
		  #30         data_a = 32'b00000000_00000001_00000000_00000010;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  		  #30 packet_in = 'b0;

		  
		  #100
		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000011_00000010_00000001,32'hffffffff,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;
		  #30 data_a = 32'b00000000_00000011_00000010_00000001;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
		  #100 
		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00001000_00000011,32'h55555555,45'b0,11'b00000000000}; //data 1
		  valid = 'b1;
		  #2 valid = 'b0;
		  #30 data_a = 32'b00000000_00000001_00001000_00000011;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  #50
		  

        #100 reset = 1;
*/


/************************************send, send, send,22cycles recv,18 cycles recv,12 cycles recv**********************/
        clk = 0;
        reset = 1;
        data_a = 32'b0;
        data_b = 32'b0;
        in = 3'b0;
        start = 0;
        data_from_mem = 32'b0;
		  valid = 'b0;
        #3 reset = 0;
		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00000000_00000010,32'h55555555,45'b0,11'b00000000000}; //data 2
		  valid = 'b1;
		  #2 valid = 'b0;
		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000011_00000010_00000001,32'hffffffff,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;

		  #30 packet_in = {5'b10000, 3'b111, 32'b00000000_00000011_00000010_00000101,32'hffffffff,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;
		  
		  
		  
		  #30         data_a = 32'b00000000_00000001_00000000_00000010;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  		 

		  
		  
		  
		  #60 data_a = 32'b00000000_00000011_00000010_00000001;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
		 
		  #60 data_a = 32'b00000000_00000001_00001000_00000011;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  #50
		  

		  #60 data_a = 32'b00000000_00000011_00000010_00000101;
        data_b = 32'b00000000_00_00000000000_00000000011;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
        #60 data_a = 32'b11111111_00000011_00000010_00000001;
        data_b = 32'b00000000_00_00000000000_00000000100;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
		
		  
		  #300 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00001000_00000011,32'h55555555,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;		  
		  
		  
        #100 reset = 1;
		  
/****************************************************/
/****************************************recv recv recv,23 cycles send,17 cycles send,11 cycles send*************************************/
/*        clk = 0;
        reset = 1;
        data_a = 32'b0;
        data_b = 32'b0;
        in = 3'b0;
        start = 0;
        data_from_mem = 32'b0;
		  valid = 'b0;
        #5 reset = 0;

		  
		  
		  
		  
		  #30         data_a = 32'b00000000_00000001_00000000_00000010;
        data_b = 32'b00000000_00_01000000001_00000000001;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  		 

		  
		  
		  
		  #60 data_a = 32'b00000000_00000011_00000010_00000001;
        data_b = 32'b00000000_00_01000000001_00000000111;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
		 
		  #60 data_a = 32'b00000000_00000001_00001000_00000011;
        data_b = 32'b00000000_00_01000000001_00000000011;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  #50
		  
		  
		  #100 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00000000_00000010,32'h55555555,45'b0,11'b00000000000}; //data 2
		  valid = 'b1;
		  #2 valid = 'b0;
		  #60 packet_in = {5'b10000, 3'b111, 32'b00000000_00000011_00000010_00000001,32'hffffffff,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;
		  #60 packet_in = {5'b10000, 3'b111, 32'b00000000_00000001_00001000_00000011,32'h55555555,45'b0,11'b00000000000}; //data 0
		  valid = 'b1;
		  #2 valid = 'b0;

		  
		  #60 data_a = 32'b11111111_00000011_00000010_00000001;
        data_b = 32'b00000000_00_00000000000_00000000011;
        in = 3'b111;
        start = 1;
		  #2 start = 0;
		  
		  
		  
		  
        #100 reset = 1;
		  
		  
*/		  
		  
		  
end
endmodule
