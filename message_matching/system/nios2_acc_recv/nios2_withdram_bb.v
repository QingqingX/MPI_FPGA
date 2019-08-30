
module nios2_withdram (
	acc_0_packet_out_packet,
	acc_0_packet_out_valid,
	clk_clk,
	nic_recv_0_network_data_comes_here_packet,
	nic_recv_0_network_data_comes_here_valid,
	reset_reset_n);	

	output	[127:0]	acc_0_packet_out_packet;
	output		acc_0_packet_out_valid;
	input		clk_clk;
	input	[127:0]	nic_recv_0_network_data_comes_here_packet;
	input		nic_recv_0_network_data_comes_here_valid;
	input		reset_reset_n;
endmodule
