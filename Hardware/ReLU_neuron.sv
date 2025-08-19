`timescale 1ns / 1ps

module ReLU #(parameter PREV_LAYER = 5, parameter NEXT_LAYER = 4) (
    input logic [15:0] weigted_inputs[PREV_LAYER],
    input logic [15:0] bias,
    input logic [15:0] weights[NEXT_LAYER]
    output logic [15:0] weighted_outputs[NEXT_LAYER]
    );
    
    logic [15:0] weighted_sum, biased_sum, activation

    summation sum (.Num_Inputs(PREV_LAYER)) (
        .din(weighted_inputs),
        .dout(weighted_sum)
    );

    summation bias_accounting (.Num_Inputs(2)) (
        .din({weighted_sum, bias}),
        .dout(biased_sum)
    );

    assign activation = (biased_sum[15]) ? 0 : biased_sum;

    multiplier weighting[NEXT_LAYER] (
        .din_a(activation),
        .din_b(weights),
        .dout(weighted_outputs)
    );
endmodule
