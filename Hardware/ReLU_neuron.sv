`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 04/23/2025 04:35:04 PM
// Design Name: 
// Module Name: keys
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


module keys(
    input  logic [31:0]  keycode,
    input  logic pxl_clk, 
    output logic left,
    output logic down,
    output logic up,
    output logic right
    );
    
    always_ff @ (posedge pxl_clk) begin
        if (keycode[15:8] == 'h50 || keycode[7:0] == 'h50 || keycode[23:16] == 'h50 || keycode[31:24] == 'h50) begin
            left = 1'b1;
        end
        else begin
            left = 1'b0;
        end
    end
    
    always_ff @ (posedge pxl_clk) begin
        if (keycode[15:8] == 'h51 || keycode[7:0] == 'h51 || keycode[23:16] == 'h51 || keycode[31:24] == 'h51) begin
            down = 1'b1;
        end
        else begin
            down = 1'b0;
        end
    end
    
    always_ff @ (posedge pxl_clk) begin
        if (keycode[15:8] == 'h52 || keycode[7:0] == 'h52 || keycode[23:16] == 'h52 || keycode[31:24] == 'h52) begin
            up = 1'b1;
        end
        else begin
            up = 1'b0;
        end
    end
    
    always_ff @ (posedge pxl_clk) begin
        if (keycode[15:8] == 'h4F || keycode[7:0] == 'h4F || keycode[23:16] == 'h4F || keycode[31:24] == 'h4F) begin
            right = 1'b1;
        end
        else begin
            right = 1'b0;
        end
    end
    
endmodule
