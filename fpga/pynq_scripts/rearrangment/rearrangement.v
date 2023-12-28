`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/07/2023 04:20:49 PM
// Design Name: 
// Module Name: rearrangement
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


module rearrangement(
    input aclk,
    input aresetn,

    input [383:0] s_axis_tdata, // 4x4_tile, stride between row = 96
    input s_axis_tvalid,
    output reg s_axis_tready,
    
    output reg [95:0] m_axis_tdata, // 4 pixels in row order
    output reg m_axis_tvalid,
    output reg m_axis_tlast,
    output reg [11:0] m_axis_tkeep, // 96 bits = 12 bytes
    input  m_axis_tready
    );
    // No double buffer for now
    
    reg state; // 0: taking input, 1: outputing rows
    reg [8:0] saved_tile_number; // 0~480, counting the input
    reg [1:0] row_idx;     // 0~3
    reg [8:0] sent_in_row_counter; // 0~480, counting the output of every row
    reg [8:0] sent_row_counter; // 0 ~ 270, counting the outputed row
    
    // bram to buffer 4 row 
    reg [95:0] bram0 [479:0];
    reg [95:0] bram1 [479:0];
    reg [95:0] bram2 [479:0];
    reg [95:0] bram3 [479:0];
    
    wire [383:0] bgr_data;
    

    // RGB to BGR rearrangement because pynq use BGR fomat
    generate
        genvar i;
        for(i=0; i < 16; i=i+1) begin : rgb2bgr
            assign bgr_data [i*24+23 : i*24] = {s_axis_tdata[i*24+7 : i*24], s_axis_tdata[i*24+15 : i*24+8], s_axis_tdata[i*24+23 : i*24+16]};
        end   
    endgenerate    
    
    always @(posedge aclk) begin
        if(!aresetn) begin
            m_axis_tvalid <= 0;
            m_axis_tdata <= 0;
            m_axis_tlast <= 0;
            m_axis_tkeep <= 0;
            s_axis_tready <= 1; //ready for first input
            
            state <= 0;
            saved_tile_number <= 0;
            row_idx <= 0;
            sent_in_row_counter <= 0;
            sent_row_counter <= 0;
            
            
            // we do not need to reset bram?
        end
        else begin
            if(state==0) begin
            // taking the input
                m_axis_tvalid <= 0;
                if (s_axis_tvalid) begin
                    bram0[saved_tile_number] <= bgr_data[95:0];
                    bram1[saved_tile_number] <= bgr_data[191:96];
                    bram2[saved_tile_number] <= bgr_data[287:192];
                    bram3[saved_tile_number] <= bgr_data[383:288];
                    
                    if(saved_tile_number == 9'd479) begin
                        saved_tile_number <= 0; // done collecting
                        s_axis_tready <= 0; // stop consuming
                        state <= 1; // start output
                    end
                    else begin
                        saved_tile_number <= saved_tile_number + 1;
                        s_axis_tready <= 1; // keep taking input
                        state <= 0; // keep taking input
                    end
                end
            end
            else begin
            // writing output
                if (m_axis_tready) begin
                    if (row_idx==2'd3 && sent_in_row_counter ==9'd479) begin
                        state <= 0; // back to comsuming
                        s_axis_tready <= 1; // back to comsuming
                        row_idx <= 0;
                        sent_in_row_counter <= 0;
                        m_axis_tkeep <= {12{1'b1}};
                        m_axis_tvalid <= 1;
                        
                        if(sent_row_counter == 9'd269) begin
                            m_axis_tlast <= 1;
                            sent_row_counter <= 0;
                        end
                        else begin
                            m_axis_tlast <= 0;
                            sent_row_counter <= sent_row_counter + 1;
                        end
                    end
                    else if(sent_in_row_counter == 9'd479) begin
                        state <= 1; //  keep outputing
                        s_axis_tready <= 0; // 
                        row_idx <= row_idx + 1;
                        sent_in_row_counter <= 0;
                        m_axis_tlast <= 0;
                        m_axis_tkeep <= {12{1'b1}};
                        m_axis_tvalid <= 1;
                    end
                    else begin
                        state <= 1; // 
                        s_axis_tready <= 0; // 
                        row_idx <= row_idx;
                        sent_in_row_counter <= sent_in_row_counter + 1;
                        m_axis_tlast <= 0;
                        m_axis_tkeep <= {12{1'b1}};
                        m_axis_tvalid <= 1;
                    end
                
                    case(row_idx)
                        2'd0: 
                            begin
                               m_axis_tdata <= bram0[sent_in_row_counter];
                            end
                        2'd1:
                            begin
                               m_axis_tdata <= bram1[sent_in_row_counter];
                            end
                        2'd2:
                            begin
                               m_axis_tdata <= bram2[sent_in_row_counter];
                            end
                        default:
                            begin
                               m_axis_tdata <= bram3[sent_in_row_counter];
                            end
                    endcase
                end
            end
        end
    end  
    
endmodule











