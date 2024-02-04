`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/03/2024 08:55:28 PM
// Design Name: 
// Module Name: simulation
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module tester_verilog_tb();

// Inputs
   wire clk_0;
   wire [31:0] error_nums_0_tdata;
   wire error_nums_0_tvalid;

   reg [31:0] s_axi_control_0_araddr;
   wire s_axi_control_0_arready;
   reg s_axi_control_0_arvalid;
   reg [31:0] s_axi_control_0_awaddr;
   wire s_axi_control_0_awready;
   reg s_axi_control_0_awvalid;

   reg s_axi_control_0_bready;
   wire [1:0] s_axi_control_0_bresp;
   wire s_axi_control_0_bvalid;

   wire [31:0] s_axi_control_0_rdata;
   reg s_axi_control_0_rready;
   wire [1:0] s_axi_control_0_rresp;
   wire s_axi_control_0_rvalid;

   reg [31:0] s_axi_control_0_wdata;
   wire s_axi_control_0_wready;
   reg [3:0] s_axi_control_0_wstrb = 4'b1111;
   reg s_axi_control_0_wvalid;

   reg [31:0] s_axi_control_r_0_araddr;
   wire s_axi_control_r_0_arready;
   reg s_axi_control_r_0_arvalid;

   reg [31:0] s_axi_control_r_0_awaddr;
   wire s_axi_control_r_0_awready;
   reg s_axi_control_r_0_awvalid;

   reg s_axi_control_r_0_bready;
   wire [1:0] s_axi_control_r_0_bresp;
   wire s_axi_control_r_0_bvalid;

   wire [31:0] s_axi_control_r_0_rdata;
   reg s_axi_control_r_0_rready;
   wire [1:0] s_axi_control_r_0_rresp;
   wire s_axi_control_r_0_rvalid;

   reg [31:0] s_axi_control_r_0_wdata;
   wire s_axi_control_r_0_wready;
   reg [3:0] s_axi_control_r_0_wstrb;
   reg s_axi_control_r_0_wvalid;

   wire sync_rst_0;
   reg ap_start_0 = 1;

design_1_wrapper u_design_1_wrapper (
    .clk_0(clk_0),
    .error_nums_0_tdata(error_nums_0_tdata),
    .error_nums_0_tvalid(error_nums_0_tvalid),
    .s_axi_control_0_araddr(s_axi_control_0_araddr),
    .s_axi_control_0_arready(s_axi_control_0_arready),
    .s_axi_control_0_arvalid(s_axi_control_0_arvalid),
    .s_axi_control_0_awaddr(s_axi_control_0_awaddr),
    .s_axi_control_0_awready(s_axi_control_0_awready),
    .s_axi_control_0_awvalid(s_axi_control_0_awvalid),
    .s_axi_control_0_bready(s_axi_control_0_bready),
    .s_axi_control_0_bresp(s_axi_control_0_bresp),
    .s_axi_control_0_bvalid(s_axi_control_0_bvalid),
    .s_axi_control_0_rdata(s_axi_control_0_rdata),
    .s_axi_control_0_rready(s_axi_control_0_rready),
    .s_axi_control_0_rresp(s_axi_control_0_rresp),
    .s_axi_control_0_rvalid(s_axi_control_0_rvalid),
    .s_axi_control_0_wdata(s_axi_control_0_wdata),
    .s_axi_control_0_wready(s_axi_control_0_wready),
    .s_axi_control_0_wstrb(s_axi_control_0_wstrb),
    .s_axi_control_0_wvalid(s_axi_control_0_wvalid),
    .s_axi_control_r_0_araddr(s_axi_control_r_0_araddr),
    .s_axi_control_r_0_arready(s_axi_control_r_0_arready),
    .s_axi_control_r_0_arvalid(s_axi_control_r_0_arvalid),
    .s_axi_control_r_0_awaddr(s_axi_control_r_0_awaddr),
    .s_axi_control_r_0_awready(s_axi_control_r_0_awready),
    .s_axi_control_r_0_awvalid(s_axi_0_awvalid),
    .s_axi_control_r_0_bready(s_axi_control_r_0_bready),
    .s_axi_control_r_0_bresp(s_axi_control_r_0_bresp),
    .s_axi_control_r_0_bvalid(s_axi_control_r_0_bvalid),
    .s_axi_control_r_0_rdata(s_axi_control_r_0_rdata),
    .s_axi_control_r_0_rready(s_axi_control_r_0_rready),
    .s_axi_control_r_0_rresp(s_axi_control_r_0_rresp),
    .s_axi_control_r_0_rvalid(s_axi_control_r_0_rvalid),
    .s_axi_control_r_0_wdata(s_axi_control_r_0_wdata),
    .s_axi_control_r_0_wready(s_axi_control_r_0_wready),
    .s_axi_control_r_0_wstrb(s_axi_control_r_0_wstrb),
    .s_axi_control_r_0_wvalid(s_axi_control_r_0_wvalid),
    .sync_rst_0(sync_rst_0),
    .ap_start_0(ap_start_0)
    );

// State for managing the transaction sequence
reg [1:0] state = 0;

reg start_transaction = 1;

// AXI write operation control
always @(posedge clk_0) begin
    if(~sync_rst_0) begin
        s_axi_control_0_araddr <= 0;
        s_axi_control_0_arvalid <= 0;
        s_axi_control_0_awaddr <= 0;
        s_axi_control_0_awvalid <= 0;
        s_axi_control_0_wdata <= 0;
        s_axi_control_0_wvalid <= 0;
        s_axi_control_0_wstrb <= 4'b1111;
        s_axi_control_0_bready <= 0;
        s_axi_control_0_rready <= 0;
    end
    else begin
        case (state)
            0: begin
                if (start_transaction) begin
                    s_axi_control_0_awaddr <= 32'h10; // Set write address
                    s_axi_control_0_wdata <= 3742;  // Set write data
                    s_axi_control_0_awvalid <= 1;
                    s_axi_control_0_wvalid <= 1;
                    s_axi_control_0_bready <= 1; // Ready to accept response
                    state <= 1;
                end
            end
            1: begin // Wait for AWREADY and WREADY
                if (s_axi_control_0_awready) begin
                    s_axi_control_0_awvalid <= 0;
                end
                if (s_axi_control_0_wready) begin
                    s_axi_control_0_wvalid <= 0;
                end
                if (s_axi_control_0_awvalid==0 && s_axi_control_0_wvalid==0) begin
                    state <= 2; // Move to next state
                end
            end
            2: begin // Wait for BVALID
                if (s_axi_control_0_bvalid) begin
                    s_axi_control_0_bready <= 0; // Transaction complete
                    state <= 0; // Reset state for potential next transaction
                    start_transaction <= 0; // Reset start signal
                end
            end
        endcase
    end
end


endmodule
