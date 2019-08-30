// (C) 2001-2015 Altera Corporation. All rights reserved.
// Your use of Altera Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License Subscription 
// Agreement, Altera MegaCore Function License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


// $Id: //acds/rel/15.1/ip/merlin/altera_customins_xconnect/altera_customins_xconnect.sv.terp#1 $
// $Revision: #1 $
// $Date: 2015/08/09 $
// $Author: swbranch $

// -------------------------------------------------------
// Custom Instruction Interconnect
//
// -------------------------------------------------------

`timescale 1 ns / 1 ns


module nios2_cpu_custom_instruction_master_comb_xconnect
(
    // -------------------
    // Custom instruction masters
    // -------------------
    output [31 : 0] ci_master0_dataa,
    output [31 : 0] ci_master0_datab,
    input  [31 : 0] ci_master0_result,
    output [ 7 : 0] ci_master0_n,
    output          ci_master0_readra,
    output          ci_master0_readrb,
    output          ci_master0_writerc,
    output [ 4 : 0] ci_master0_a,
    output [ 4 : 0] ci_master0_b,
    output [ 4 : 0] ci_master0_c,
    output [31 : 0] ci_master0_ipending,
    output          ci_master0_estatus,

    output [31 : 0] ci_master1_dataa,
    output [31 : 0] ci_master1_datab,
    input  [31 : 0] ci_master1_result,
    output [ 7 : 0] ci_master1_n,
    output          ci_master1_readra,
    output          ci_master1_readrb,
    output          ci_master1_writerc,
    output [ 4 : 0] ci_master1_a,
    output [ 4 : 0] ci_master1_b,
    output [ 4 : 0] ci_master1_c,
    output [31 : 0] ci_master1_ipending,
    output          ci_master1_estatus,

    output [31 : 0] ci_master2_dataa,
    output [31 : 0] ci_master2_datab,
    input  [31 : 0] ci_master2_result,
    output [ 7 : 0] ci_master2_n,
    output          ci_master2_readra,
    output          ci_master2_readrb,
    output          ci_master2_writerc,
    output [ 4 : 0] ci_master2_a,
    output [ 4 : 0] ci_master2_b,
    output [ 4 : 0] ci_master2_c,
    output [31 : 0] ci_master2_ipending,
    output          ci_master2_estatus,


    // -------------------
    // Custom instruction slave
    // -------------------
    input  [31 : 0] ci_slave_dataa,
    input  [31 : 0] ci_slave_datab,
    output [31 : 0] ci_slave_result,
    input  [ 7 : 0] ci_slave_n,
    input           ci_slave_readra,
    input           ci_slave_readrb,
    input           ci_slave_writerc,
    input  [ 4 : 0] ci_slave_a,
    input  [ 4 : 0] ci_slave_b,
    input  [ 4 : 0] ci_slave_c,
    input  [31 : 0] ci_slave_ipending,
    input           ci_slave_estatus

);

    wire select0;
    wire select1;
    wire select2;

    // -------------------------------------------------------
    // Wire non-control signals through to each master
    // -------------------------------------------------------
    assign  ci_master0_dataa    = ci_slave_dataa;
    assign  ci_master0_datab    = ci_slave_datab;
    assign  ci_master0_n        = ci_slave_n;
    assign  ci_master0_a        = ci_slave_a;
    assign  ci_master0_b        = ci_slave_b;
    assign  ci_master0_c        = ci_slave_c;
    assign  ci_master0_ipending = ci_slave_ipending;
    assign  ci_master0_estatus  = ci_slave_estatus;

    assign  ci_master1_dataa    = ci_slave_dataa;
    assign  ci_master1_datab    = ci_slave_datab;
    assign  ci_master1_n        = ci_slave_n;
    assign  ci_master1_a        = ci_slave_a;
    assign  ci_master1_b        = ci_slave_b;
    assign  ci_master1_c        = ci_slave_c;
    assign  ci_master1_ipending = ci_slave_ipending;
    assign  ci_master1_estatus  = ci_slave_estatus;

    assign  ci_master2_dataa    = ci_slave_dataa;
    assign  ci_master2_datab    = ci_slave_datab;
    assign  ci_master2_n        = ci_slave_n;
    assign  ci_master2_a        = ci_slave_a;
    assign  ci_master2_b        = ci_slave_b;
    assign  ci_master2_c        = ci_slave_c;
    assign  ci_master2_ipending = ci_slave_ipending;
    assign  ci_master2_estatus  = ci_slave_estatus;


    // -------------------------------------------------------
    // Figure out which output is selected, and use that to
    // gate control signals
    // -------------------------------------------------------
    assign select0 = ci_slave_n >= 0 && ci_slave_n < 8;

    assign ci_master0_readra  = (select0 && ci_slave_readra);
    assign ci_master0_readrb  = (select0 && ci_slave_readrb);
    assign ci_master0_writerc = (select0 && ci_slave_writerc);

    assign select1 = ci_slave_n >= 8 && ci_slave_n < 16;

    assign ci_master1_readra  = (select1 && ci_slave_readra);
    assign ci_master1_readrb  = (select1 && ci_slave_readrb);
    assign ci_master1_writerc = (select1 && ci_slave_writerc);

    assign select2 = ci_slave_n >= 224 && ci_slave_n < 240;

    assign ci_master2_readra  = (select2 && ci_slave_readra);
    assign ci_master2_readrb  = (select2 && ci_slave_readrb);
    assign ci_master2_writerc = (select2 && ci_slave_writerc);


    // -------------------------------------------------------
    // Use the select signal to figure out which result to mux
    // back
    // -------------------------------------------------------
    assign ci_slave_result = {32{ select0 }} & ci_master0_result
         | {32{ select1 }} & ci_master1_result
         | {32{ select2 }} & ci_master2_result
    ;


endmodule


