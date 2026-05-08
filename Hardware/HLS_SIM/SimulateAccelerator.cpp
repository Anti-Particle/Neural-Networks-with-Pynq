#include <iostream>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <cmath>
#include <cstdlib>

#define MAX_ROWS 120
#define MAX_COLS 400

typedef ap_axis<32,0,0,0> stream;

void mv_dp_accel(hls::stream<stream> &in_stream,
                 hls::stream<stream> &out_stream,
                 int rows,
                 int cols,
                 int cmd);

static void push_stream(hls::stream<stream> &s, float val, bool last=false) { // Function to push weigths, bias, and input vector to AXI-Stream
    stream pkt;
    pkt.data = *((ap_uint<32>*)(&val));
    pkt.last = last;
    s.write(pkt);
}

static float pop_stream(hls::stream<stream> &s) { // Function to readout output stream of IP
    stream pkt = s.read();
    return *((float*)(&pkt.data));
}

int main() {

    const int rows = 120;
    const int cols = 400;

    float weights[MAX_ROWS][MAX_COLS];
    float bias[MAX_ROWS];
    float vector[MAX_COLS];

    float sw_out[MAX_ROWS]; // Output matricies for comparison
    float hw_out[MAX_ROWS];

    hls::stream<stream> in_stream;
    hls::stream<stream> out_stream;

    // Generate random test data
    for(int i=0;i<rows;i++){
        bias[i] = ((float)rand()/RAND_MAX)-0.5f;
        for(int j=0;j<cols;j++){
            weights[i][j] = ((float)rand()/RAND_MAX)-0.5f;
        }
    }

    for(int j=0;j<cols;j++){
        vector[j] = ((float)rand()/RAND_MAX)-0.5f;
    }

    for(int i=0;i<rows;i++){ // Compute ReLU(Wx + b) on software
        float acc = bias[i];
        for(int j=0;j<cols;j++){
            acc += vector[j] * weights[i][j];
        }
        sw_out[i] = acc > 0 ? acc : 0;
    }

    for(int i=0;i<rows;i++){ // Stream in weigths
        for(int j=0;j<cols;j++){
            push_stream(in_stream, weights[i][j]);
        }
    }

    mv_dp_accel(in_stream, out_stream, rows, cols, 0);


    for(int i=0;i<rows;i++){
        push_stream(in_stream, bias[i]); // Load bias
    }

    mv_dp_accel(in_stream, out_stream, rows, cols, 1);

    for(int j=0;j<cols;j++){
        push_stream(in_stream, vector[j]); // Load vector
    }

    
    mv_dp_accel(in_stream, out_stream, rows, cols, 2); // Run accelerator

    for(int i=0;i<rows;i++){
        hw_out[i] = pop_stream(out_stream);
    }

    // Compare results
    int errors = 0;

    for(int i=0;i<rows;i++){
        float diff = fabs(hw_out[i] - sw_out[i]);

        if(diff > 1e-3){
            std::cout << "Mismatch at " << i
                      << " HW=" << hw_out[i]
                      << " SW=" << sw_out[i]
                      << std::endl;
            errors++;
        }
    }

    if(errors == 0)
        std::cout << "TEST PASSED" << std::endl;
    else
        std::cout << "TEST FAILED: " << errors << " mismatches" << std::endl;

    return errors;
}