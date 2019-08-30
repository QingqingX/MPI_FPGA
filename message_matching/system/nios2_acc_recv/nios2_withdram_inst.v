	nios2_withdram u0 (
		.acc_0_packet_out_packet                   (<connected-to-acc_0_packet_out_packet>),                   //                   acc_0_packet_out.packet
		.acc_0_packet_out_valid                    (<connected-to-acc_0_packet_out_valid>),                    //                                   .valid
		.clk_clk                                   (<connected-to-clk_clk>),                                   //                                clk.clk
		.nic_recv_0_network_data_comes_here_packet (<connected-to-nic_recv_0_network_data_comes_here_packet>), // nic_recv_0_network_data_comes_here.packet
		.nic_recv_0_network_data_comes_here_valid  (<connected-to-nic_recv_0_network_data_comes_here_valid>),  //                                   .valid
		.reset_reset_n                             (<connected-to-reset_reset_n>)                              //                              reset.reset_n
	);

