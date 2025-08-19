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


    //Fraction Calculation
    always_comb begin
        
    end   

    //Exponent Calculation
    always_comb begin
        if (mult_zero) expo = 0;
        else
    end   

    assign dout[15] = din_a[15] ^ din_b[15];
    
endmodule
