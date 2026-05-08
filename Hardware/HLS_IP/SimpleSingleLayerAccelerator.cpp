#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define MAX_ROWS 120
#define MAX_COLS 400
#define DOTS 6
#define DOT_SLOTS 5

typedef ap_axis<32, 0, 0, 0> stream;

void load_weights(hls::stream<stream> &in_stream, int rows, int cols, float weights[MAX_ROWS][MAX_COLS]);
void load_bias(hls::stream<stream> &in_stream, int rows, float bias[MAX_ROWS]);
void load_vector(hls::stream<stream> &in_stream, int cols, float vector[MAX_COLS]);
void execute_dp_scheduler(int rows, int cols, float vector[MAX_COLS], float bias[MAX_ROWS], float weights[MAX_ROWS][MAX_COLS], float output[MAX_ROWS]);
void stream_out(hls::stream<stream> &out_stream, int rows, float output[MAX_ROWS]);

// Possible Improvements
// Vector Batching
// -- Change Vector matrix to 2D FIFO for effective batching
// Parameter Sweep for DSP and BRAM usage Maximization
// -- Done, LUT count ~35% of Max, BRAM count ~93% of Max, DSP count ~78% of Max
// Chaining of functions to replicate LeNet-5 Dense Layers
// -- Possible Pipelined structure for dense layers??
// ----- Might not enough FPGA resources
// -- Reduction of Parallelism needed to fit on board
// Function reuse
// Increasing clock speed for potentially higher data throughput


void mv_dp_accel(hls::stream<stream> &in_stream, hls::stream<stream> &out_stream, int rows, int cols, int cmd) {
    #pragma HLS INTERFACE axis port=in_stream
    #pragma HLS INTERFACE axis port=out_stream
    #pragma HLS INTERFACE s_axilite port=rows
    #pragma HLS INTERFACE s_axilite port=cols
    #pragma HLS INTERFACE s_axilite port=cmd
    #pragma HLS INTERFACE s_axilite port=return
   
    static float weights[MAX_ROWS][MAX_COLS];
    static float vector[MAX_COLS];
    static float bias[MAX_ROWS];
    static float output[MAX_ROWS];

    #pragma HLS BIND_STORAGE variable=weights type=ram_2p impl=bram
    #pragma HLS ARRAY_PARTITION variable=weights dim=1 type=cyclic factor=DOTS
    #pragma HLS ARRAY_PARTITION variable=weights dim=2 type=cyclic factor=DOT_SLOTS

    #pragma HLS ARRAY_PARTITION variable=vector dim=1 type=cyclic factor=DOT_SLOTS
    #pragma HLS ARRAY_PARTITION variable=bias dim=1 type=cyclic factor=DOTS
    #pragma HLS ARRAY_PARTITION variable=output dim=1 type=cyclic factor=DOTS

    switch (cmd) { // Executes a certain function based PS input comand
        case(0) : {
            load_weights(in_stream, rows, cols, weights);
            break;
        }
        case(1) : {
            load_bias(in_stream, rows, bias);
            break;
        }
        case(2) : {
            load_vector(in_stream, cols, vector);
            execute_dp_scheduler(rows, cols, vector, bias, weights, output);
            stream_out(out_stream, rows, output);
            break;
        }
    }
}

void load_weights(hls::stream<stream> &in_stream, int rows, int cols, float weights[MAX_ROWS][MAX_COLS]) { // Loads an AXI Stream of rows*cols float32 weights in row-major order
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j < MAX_COLS; j++) {
            #pragma HLS PIPELINE II=1
            if (i < rows && j < cols) {
                stream tmp = in_stream.read();
                weights[i][j] = *((float*)(&tmp.data));
            } else {
                weights[i][j] = 0.0f;
            }
        }
    }
}

void load_bias(hls::stream<stream> &in_stream, int rows, float bias[MAX_ROWS]) { // Loads the bias vector from an AXI Stream of rows float32s
    for(int i = 0; i < MAX_ROWS; i++){
        #pragma HLS PIPELINE II=1
        if (i < rows) {
            stream tmp = in_stream.read();
            bias[i] = *((float*)(&tmp.data));
        } else {
            bias[i] = 0.0f;
        }
    }
}

void load_vector(hls::stream<stream> &in_stream, int cols, float vector[MAX_COLS]) { // Same code as load_bias, except using cols as length parameter
    for(int i=0;i<MAX_COLS;i++){
        #pragma HLS PIPELINE II=1
        if (i < cols) {
            stream tmp = in_stream.read();
            vector[i] = *((float*)(&tmp.data));
        } else {
            vector[i] = 0.0f;
        }
    }
}

void execute_dp_scheduler(int rows, int cols,
                          float vector[MAX_COLS],
                          float bias[MAX_ROWS],
                          float weights[MAX_ROWS][MAX_COLS],
                          float output[MAX_ROWS]) { // Schedule multiple pipelined dot-product accelerators to work in parallel to compute output vector entries in batches

    const int ACC_LATENCY = 4;

    for (int i = 0; i < rows; i += DOTS) { // Schedule DOTS DP accelerators at a time
        #pragma HLS LOOP_TRIPCOUNT min=1 max=12 // Used to prevent latency estimator from using integer max
        #pragma HLS LOOP_FLATTEN off

        float acc_bins[DOTS][ACC_LATENCY];
        #pragma HLS ARRAY_PARTITION variable=acc_bins complete dim=0


        for (int d = 0; d < DOTS; d++) { // Initialize accumulation bins
            #pragma HLS UNROLL
            acc_bins[d][0] = bias[i+d];
            for (int b = 1; b < ACC_LATENCY; b++) {
                #pragma HLS UNROLL
                acc_bins[d][b] = 0.0f;
            }
        }
       

        int bin_idx = 0;
        for (int j = 0; j < cols; j += DOT_SLOTS) { // Each accelerator mul-acc DOT_SLOTS entries from weight row and input vector at a time
            #pragma HLS LOOP_TRIPCOUNT min=1 max=40 // Used to prevent latency estimator from using integer max
            #pragma HLS PIPELINE II=1
           
            for (int d = 0; d < DOTS; d++) {
                #pragma HLS UNROLL
               
                float dot_sum = 0.0f;
                for (int s = 0; s < DOT_SLOTS; s++) {
                    #pragma HLS UNROLL
                    dot_sum += vector[j+s] * weights[i+d][j+s];
                }
               
                acc_bins[d][bin_idx] += dot_sum; // Multiple accumulator bins used to allow for lower pipeline depth
            }
            bin_idx = (bin_idx == ACC_LATENCY - 1) ? 0 : bin_idx + 1;
        }


        for (int d = 0; d < DOTS; d++) { // Accumulator bins summed at the end of the row dot product computation to produce final entry in output vector
            #pragma HLS UNROLL
            float final_val = 0.0f;
           
            for (int b = 0; b < ACC_LATENCY; b++) {
                #pragma HLS UNROLL
                final_val += acc_bins[d][b];
            }
           
            output[i+d] = (final_val > 0.0f) ? final_val : 0.0f; // Relu activation func performed for minimal cost
        }
    }
}

void stream_out(hls::stream<stream> &out_stream, int rows, float output[MAX_ROWS]) { // Returns the output vector
    for (int i = 0; i < rows; i++) {
        #pragma HLS PIPELINE II=1
        stream out;
        out.data = *((ap_uint<32>*)(&output[i]));
        out.last = (i == rows-1);
        out_stream.write(out);
    }
}