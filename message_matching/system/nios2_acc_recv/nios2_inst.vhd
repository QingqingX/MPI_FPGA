	component nios2 is
		port (
			acc_0_packet_out_packet   : out std_logic_vector(127 downto 0);                    -- packet
			acc_0_packet_out_valid    : out std_logic;                                         -- valid
			acc_recv_0_pack_in_packet : in  std_logic_vector(127 downto 0) := (others => 'X'); -- packet
			acc_recv_0_pack_in_valid  : in  std_logic                      := 'X';             -- valid
			clk_clk                   : in  std_logic                      := 'X';             -- clk
			reset_reset_n             : in  std_logic                      := 'X'              -- reset_n
		);
	end component nios2;

	u0 : component nios2
		port map (
			acc_0_packet_out_packet   => CONNECTED_TO_acc_0_packet_out_packet,   --   acc_0_packet_out.packet
			acc_0_packet_out_valid    => CONNECTED_TO_acc_0_packet_out_valid,    --                   .valid
			acc_recv_0_pack_in_packet => CONNECTED_TO_acc_recv_0_pack_in_packet, -- acc_recv_0_pack_in.packet
			acc_recv_0_pack_in_valid  => CONNECTED_TO_acc_recv_0_pack_in_valid,  --                   .valid
			clk_clk                   => CONNECTED_TO_clk_clk,                   --                clk.clk
			reset_reset_n             => CONNECTED_TO_reset_reset_n              --              reset.reset_n
		);

