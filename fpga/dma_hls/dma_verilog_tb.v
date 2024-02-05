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


module dma_verilog_tb();

// Inputs
   wire clk_0;

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

   reg [31:0] s_axi_control_1_araddr;
   wire s_axi_control_1_arready;
   reg s_axi_control_1_arvalid;

   reg [31:0] s_axi_control_1_awaddr;
   wire s_axi_control_1_awready;
   reg s_axi_control_1_awvalid;

   reg s_axi_control_1_bready;
   wire [1:0] s_axi_control_1_bresp;
   wire s_axi_control_1_bvalid;

   wire [31:0] s_axi_control_1_rdata;
   reg s_axi_control_1_rready;
   wire [1:0] s_axi_control_1_rresp;
   wire s_axi_control_1_rvalid;

   reg [31:0] s_axi_control_1_wdata;
   wire s_axi_control_1_wready;
   reg [3:0] s_axi_control_1_wstrb;
   reg s_axi_control_1_wvalid;

   wire sync_rst_0;
   reg ap_start_0 = 1;

design_1_wrapper u_design_1_wrapper (
    .clk_0(clk_0),
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
    .s_axi_control_1_araddr(s_axi_control_1_araddr),
    .s_axi_control_1_arready(s_axi_control_1_arready),
    .s_axi_control_1_arvalid(s_axi_control_1_arvalid),
    .s_axi_control_1_awaddr(s_axi_control_1_awaddr),
    .s_axi_control_1_awready(s_axi_control_1_awready),
    .s_axi_control_1_awvalid(s_axi_control_1_awvalid),
    .s_axi_control_1_bready(s_axi_control_1_bready),
    .s_axi_control_1_bresp(s_axi_control_1_bresp),
    .s_axi_control_1_bvalid(s_axi_control_1_bvalid),
    .s_axi_control_1_rdata(s_axi_control_1_rdata),
    .s_axi_control_1_rready(s_axi_control_1_rready),
    .s_axi_control_1_rresp(s_axi_control_1_rresp),
    .s_axi_control_1_rvalid(s_axi_control_1_rvalid),
    .s_axi_control_1_wdata(s_axi_control_1_wdata),
    .s_axi_control_1_wready(s_axi_control_1_wready),
    .s_axi_control_1_wstrb(s_axi_control_1_wstrb),
    .s_axi_control_1_wvalid(s_axi_control_1_wvalid),
    .sync_rst_0(sync_rst_0)
    );

// State for managing the transaction sequence
reg [4:0] state0 = 0;

// axi_lite control 0 control loop
// Step 1. Write frame_offset at 0x28
// Step 2. Write start to IP
// Step 3. Done
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
        case (state0)
        // Step 1. Write frame_offset at 0x28
            0: begin
                s_axi_control_0_awaddr <= 32'h28; // Set write address
                s_axi_control_0_awvalid <= 1;

                s_axi_control_0_wdata <= 3742;  // Set write data
                s_axi_control_0_wvalid <= 1;
                s_axi_control_0_wstrb <= 4'b1111;
                state0 <= 1;
            end
            1: begin // Wait for AWREADY and WREADY
                if (s_axi_control_0_awready) begin
                    s_axi_control_0_awvalid <= 0;
                end
                if (s_axi_control_0_wready) begin
                    s_axi_control_0_wvalid <= 0;
                end
                if (s_axi_control_0_awvalid==0 && s_axi_control_0_wvalid==0) begin
                    state0 <= 2; // Move to next state
                    s_axi_control_0_bready <= 1; // Ready to accept response
                end
            end
            2: begin // Wait for BVALID
                if (s_axi_control_0_bvalid) begin
                    s_axi_control_0_bready <= 0; // Transaction complete
                    state0 <= 3;
                end
            end
        // Step2 Write start to IP
            3: begin
                s_axi_control_0_awaddr <= 32'h00; // Set write address
                s_axi_control_0_awvalid <= 1;
                s_axi_control_0_wdata <= 1;  // Set start
                s_axi_control_0_wvalid <= 1;
                s_axi_control_0_wstrb <= 4'b0001;
                state0 <= 4;
            end
            4: begin // Wait for AWREADY and WREADY
                if (s_axi_control_0_awready) begin
                    s_axi_control_0_awvalid <= 0;
                end
                if (s_axi_control_0_wready) begin
                    s_axi_control_0_wvalid <= 0;
                end
                if (s_axi_control_0_awvalid==0 && s_axi_control_0_wvalid==0) begin
                    state0 <= 5; // Move to next state
                    s_axi_control_0_bready <= 1; // Ready to accept response
                end
            end
            5: begin // Wait for BVALID
                if (s_axi_control_0_bvalid) begin
                    s_axi_control_0_bready <= 0; // Transaction complete
                    state0 <= 6;
                end
            end
        // Step 3. Done
            6: begin
                state0 <= 6;
            end
        endcase
    end
end


reg [31:0] error_num;
reg [31:0] acc_error_num, acc_frame_num;
reg [4:0] state1 = 0;

localparam test_frame_num = 10;

// AXI read operation control s_axi_control_1_araddr = 0x10 (error_num)
// AXI Write operation control s_axi_control_1_awaddr = 0x20 (test_frame_num)
// Interface Sequence: 
// 1. Write test_frame_num 0x20
// 2. start the IP
// 3. Wait for the IP to finish
// 4. Read error_num 0x10
// 5. Accumulate error_num and test_frame_num4
// 6. Go back to step 1
always @(posedge clk_0) begin
    if(~sync_rst_0) begin
        error_num <= 0;
        s_axi_control_1_araddr <= 0;
        s_axi_control_1_arvalid <= 0;
        s_axi_control_1_awaddr <= 0;
        s_axi_control_1_awvalid <= 0;
        s_axi_control_1_wdata <= 0;
        s_axi_control_1_wvalid <= 0;
        s_axi_control_1_wstrb <= 4'b1111;
        s_axi_control_1_bready <= 0;
        s_axi_control_1_rready <= 0;
        acc_error_num <= 0;
        acc_frame_num <= 0;
    end
    else begin
        case (state1)
        // 1. Write test_frame_num
            0: begin
                s_axi_control_1_awaddr <= 32'h20; // Set write address
                s_axi_control_1_awvalid <= 1;
                s_axi_control_1_wdata <= test_frame_num;  // Set write data
                s_axi_control_1_wvalid <= 1;
                s_axi_control_1_wstrb <= 4'b1111;
                state1 <= 1;
            end
            1: begin // Wait for AWREADY and WREADY
                if (s_axi_control_1_awready) begin
                    s_axi_control_1_awvalid <= 0;
                end
                if (s_axi_control_1_wready) begin
                    s_axi_control_1_wvalid <= 0;
                end
                if (s_axi_control_1_awvalid==0 && s_axi_control_1_wvalid==0) begin
                    state1 <= 2; // Move to next state
                    s_axi_control_1_bready <= 1; // Ready to accept response
                end
            end
            2: begin // Wait for BVALID
                if (s_axi_control_1_bvalid) begin
                    s_axi_control_1_bready <= 0; // Transaction complete
                    state1 <= 3; // Reset state for potential next transaction
                end
            end
        // 2. start the IP
            3: begin
                s_axi_control_1_awaddr <= 32'h00; // Set write address
                s_axi_control_1_awvalid <= 1;
                s_axi_control_1_wdata <= 1;  // Set start
                s_axi_control_1_wvalid <= 1;
                s_axi_control_1_wstrb <= 4'b0001;
                state1 <= 4;
            end
            4: begin // Wait for AWREADY and WREADY
                if (s_axi_control_1_awready) begin
                    s_axi_control_1_awvalid <= 0;
                end
                if (s_axi_control_1_wready) begin
                    s_axi_control_1_wvalid <= 0;
                end
                if (s_axi_control_1_awvalid==0 && s_axi_control_1_wvalid==0) begin
                    state1 <= 5; // Move to next state
                    s_axi_control_1_bready <= 1; // Ready to accept response
                end
            end
            5: begin // Wait for BVALID
                if (s_axi_control_1_bvalid) begin
                    s_axi_control_1_bready <= 0; // Transaction complete
                    state1 <= 6;
                end
            end
        // Wait for IP done
            6: begin
                s_axi_control_1_araddr <= 32'h00; // Set read address
                s_axi_control_1_arvalid <= 1;
                state1 <= 7;
            end
            7: begin // Wait for ARREADY
                if (s_axi_control_1_arready) begin
                    s_axi_control_1_arvalid <= 0;
                    state1 <= 8;
                    s_axi_control_1_rready <= 1; // Ready to accept response
                end
            end
            8: begin // Wait for RVALID
                if (s_axi_control_1_rvalid) begin
                    if (s_axi_control_1_rdata[2]) begin // if idle is high go to next state
                        state1 <= 9;
                    end
                    else begin
                        state1 <= 6; // if idle is low Go back and re-read
                    end
                    s_axi_control_1_rready <= 0;
                end
            end
        // Read error_num
            9: begin
                s_axi_control_1_araddr <= 32'h10; // Set read address
                s_axi_control_1_arvalid <= 1;
                state1 <= 10;
            end
            10: begin // Wait for ARREADY
                if (s_axi_control_1_arready) begin
                    s_axi_control_1_arvalid <= 0;
                    state1 <= 11;
                    s_axi_control_1_rready <= 1; // Ready to accept response
                end
            end
            11: begin // Wait for RVALID
                if (s_axi_control_1_rvalid) begin
                    state1 <= 0;
                    error_num <= s_axi_control_1_rdata;
                    s_axi_control_1_rready <= 0;
                    state1 <= 12;
                end
            end
        // Accumulate error_num and test_frame_num, go back to step 1
            12: begin
                acc_error_num <= error_num + acc_error_num;
                acc_frame_num <= test_frame_num + acc_frame_num;
                state1 <= 0;
            end
        endcase
    end
end




endmodule
