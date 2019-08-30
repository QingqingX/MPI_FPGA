
module nios2 (
	acc_0_packet_out_packet,
	acc_0_packet_out_valid,
	acc_recv_0_pack_in_packet,
	acc_recv_0_pack_in_valid,
	clk_clk,
	reset_reset_n);	

	output	[127:0]	acc_0_packet_out_packet;
	output		acc_0_packet_out_valid;
	input	[127:0]	acc_recv_0_pack_in_packet;
	input		acc_recv_0_pack_in_valid;
	input		clk_clk;
	input		reset_reset_n;
endmodule
