/*arbiter has a avalon-master port writing to S2 of the main CPU memory,
* and the other two parts are from send and recv,
* arbitration is necessary between the send and the recv*/
module arbit  #(
		parameter cpu_width = 32,
		parameter packetizer_width = 128,
		parameter data_width = 32,
		parameter mem_width = 32,
		parameter mem_depth = 11,
		parameter threshold  = 1,
		parameter SIZE = 3
)(
	   /*******connect to s2 of memory for writing(recv) and reading(send) **********/
		input  wire 		 waitrequest, //not used 
		output wire        write,
		output wire [data_width-1:0]   writedata,   
		output wire [19:0] mem_addr,
		input  wire [data_width-1:0]   readdata,   
		output wire chipselect,
		output wire read, //same as chipselect here, only for interfacing as avalon memory mapped master

		////////////////////////acc_send//////////////added for datatype, data fetched from CPU/////////////
		
		input wire [19:0] mem_addr_s,
		output  wire [data_width-1:0]   data_from_mem_s,   
		input wire read_s, //same as chipselect here, only for interfacing as avalon memory mapped master
		//output  wire hold,
		
	
		////////////////////////acc_recv////////////////////////////
		input wire        write_r,
		input wire [19:0] write_addr_r,
		input wire [data_width-1:0]   data_to_mem_r,
		///////////////////////avalon requirements/////////////////
		input wire reset,     //       reset_1.reset
		input wire clk                       //         clock.clk
);

assign write = write_r;
assign read  = 'b0;
assign mem_addr = write_r ? write_addr_r[19:0] : mem_addr_s;    //address of NIOSII memory is from zero to 32'hf9fff  
assign writedata = data_to_mem_r; 
assign data_from_mem_s = readdata;
assign chipselect = 1'b1;

endmodule