#include "dma.h"
#include <iostream>
#include <hls_stream.h>

int main() {
    // Test data sizes
    const int tx_len = 10;
    const int rx_len = 10;

    // Create input and output arrays
    data_t axi_rd[tx_len];
    data_t axi_wr[rx_len];

    // Initialize input data
    for (int i = 0; i < tx_len; i++) {
        axi_rd[i] = static_cast<data_t>(rand());;
    }

    // Create streams
    hls::stream<data_t> axis_mm2s, axis_s2mm;

    // Call the function
    axi_dma(axi_rd, axi_wr, axis_mm2s, axis_s2mm, tx_len, rx_len);

    // Check results
    bool error = false;
    for (int i = 0; i < rx_len; i++) {
        if (axi_wr[i] != axi_rd[i]) {
            std::cerr << "Error: Mismatch at index " << i << std::endl;
            error = true;
        }
    }

    if (!error) {
        std::cout << "Test passed successfully." << std::endl;
    }

    return 0;
}