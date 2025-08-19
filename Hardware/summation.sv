`timescale 1ns / 1ps

module summation #(parameter Num_Inputs)(
    input  logic [15:0] din [Num_Inputs],
    output logic [15:0]  dout
    );
    
    logic [4:0] expo;
    logic [9:0] frac;
    logic sign;

    always_comb begin
        expo = din[0][9:0];
        frac = din[0][14:10];
        sign = din[0][15];
        for (int i = 1; i < Num_Inputs; i = i + 1) begin
            if (din[i][14:10] > frac) begin
                frac = din[i][14:10];
            end
        end
    end
    
endmodule
