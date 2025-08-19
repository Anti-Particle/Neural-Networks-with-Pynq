module multiplier(
    input  logic [15:0] din_a, din_b,
    output logic [15:0] dout
    );

    logic [21:0] fraction_prod;
    logic [9:0] frac;
    logic [4:0] expo;
    logic mult_zero;

    assign fraction_prod = {1b'1, din_a[9:0]} * {1b'1, din_a[9:0]};
    assign mult_zero = (din_a[14:10] == 0 && din_a[9:0] == 0) | (din_b[14:10] == 0 && din_b[9:0] == 0) ? 1 : 0;
    assign frac = (mult_zero) ? 0 : fraction_prod[21:12];
    assign expo = (mult_zero) ? 0 : ((din_a[14:10] - 127) + (din_a[14:10] - 127) + 127);

    assign dout[15] = din_a[15] ^ din_b[15];
    assign dout[14:10] = expo;
    assign dout[9:0] = frac;
    
endmodule
