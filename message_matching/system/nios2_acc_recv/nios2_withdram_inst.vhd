	component nios2_withdram is
		port (
			acc_0_packet_out_packet                   : out std_logic_vector(127 downto 0);                    -- packet
			acc_0_packet_out_valid                    : out std_logic;                                         -- valid
			clk_clk                                   : in  std_logic                      := 'X';             -- clk
			nic_recv_0_network_data_comes_here_packet : in  std_logic_vector(127 downto 0) := (others => 'X'); -- packet
			nic_recv_0_network_data_comes_here_valid  : in  std_logic                      := 'X';             -- valid
			reset_reset_n                             : in  std_logic                      := 'X'              -- reset_n
		);
	end component nios2_withdram;

	u0 : component nios2_withdram
		port map (
			acc_0_packet_out_packet                   => CONNECTED_TO_acc_0_packet_out_packet,                   --                   acc_0_packet_out.packet
			acc_0_packet_out_valid                    => CONNECTED_TO_acc_0_packet_out_valid,                    --                                   .valid
			clk_clk                                   => CONNECTED_TO_clk_clk,                                   --                                clk.clk
			nic_recv_0_network_data_comes_here_packet => CONNECTED_TO_nic_recv_0_network_data_comes_here_packet, -- nic_recv_0_network_data_comes_here.packet
			nic_recv_0_network_data_comes_here_valid  => CONNECTED_TO_nic_recv_0_network_data_comes_here_valid,  --                                   .valid
			reset_reset_n                             => CONNECTED_TO_reset_reset_n                              --                              reset.reset_n
		);

