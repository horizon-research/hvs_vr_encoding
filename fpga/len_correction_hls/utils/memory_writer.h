#include "precomputation/precompute_constant.h"
#include "utils/types.h"
#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_math.h>


#ifndef MEMORY_W
#define MEMORY_W // Prevent duplicate definition

namespace vr_prototype
{
    class Memory_writer
    {
         public:
            const auto _inserts = inserts;

            void operator() (hls::stream<Memory_write_t> &memory_write_stream, hls::stream<Pixel> &din){
                #pragma HLS DATAFLOW
                hls::stream<row_trigger_t> row_trigger_stream;
                inserts_decoder(row_trigger_stream);
                row_writer(memory_write_stream, din, row_trigger_stream);
            }

            void inserts_decoder(hls::stream<row_trigger_t> &row_trigger_stream) {
                ap_uint<11> acc_yield = 0;
                for (int i = 0; i < 1080; i++) {
                    #pragma HLS PIPELINE II=1 rewind
                    for (int j = 0; j < _inserts[i] + 1; j++) {
                        if (_inserts[i] == 0) { // first count is only for adding yield
                            acc_yield ++;
                        }
                        else {
                            row_trigger_t row_trigger;
                            if (j == _inserts[i]) {
                                row_trigger.yiled_num = acc_yield;
                                acc_yield = 0;
                            }
                            else {
                                row_trigger.yiled_num = 0;
                            }
                            row_trigger_stream.write(row_trigger);
                        }
                    }
                }
            }

            void row_writer( hls::stream<Memory_write_t> &memory_write_stream, hls::stream<Pixel> &din, hls::stream<row_trigger_t> &row_trigger_stream) {
                ap_uint<11> yield_num = 0;
                for ( int i = 0; i < 1080; i ++)
                {
                    for ( int j = 0; j < 960; j++)
                    {
                        #pragma HLS PIPELINE II=1 rewind
                        if (j == 0) {
                            row_trigger_t row_trigger = row_trigger_stream.read();
                            yield_num = row_trigger.yiled_num;
                        }
                        Memory_write_t memory_write;
                        memory_write.data = din.read();
                        memory_write.rows = i;
                        memory_write.cols = j;
                        if (j == 959) {
                            memory_write.yield_num = yield_num;
                        }
                        else {
                            memory_write.yield_num = 0;
                        }
                        memory_write_stream.write(memory_write);
                    }
                }

            }
    }


}

#endif