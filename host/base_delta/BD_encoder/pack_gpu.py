from numba import cuda
import numpy as np
import time
import cupy as cp

@cuda.jit('void(uint32[:], uint8[:], uint32[:], uint8[:])')
def pack_data_cuda(packed, values, positions, lengths):
    idx = cuda.grid(1)
    if idx < values.size:
        value = values[idx]
        length = lengths[idx]
        bit_pos = positions[idx]

        for j in range(length):
            if value & (1 << (length - j - 1)):
                byte_index = bit_pos // 8
                bit_index = bit_pos % 8
                added_value = 1 << (7 - bit_index)
                cuda.atomic.add(packed, byte_index, added_value)

            bit_pos += 1

def pack_data_cuda_wrapper(values, lengths):
    positions = cp.cumsum(lengths, dtype=cp.uint32)
    positions = cp.concatenate((cp.zeros(1, dtype=cp.uint32), positions[:-1]))

    cum_sum = positions[-1] + lengths[-1]

    # compute total number of bits / bytes
    total_bits = cum_sum
    num_bytes = (total_bits + 7) // 8

    # config cuda threads and blocks
    threadsperblock = 32
    blockspergrid = (values.size + (threadsperblock - 1)) // threadsperblock

    # create an empty byte array, here we use uint32 to store the data instead of uint8 becasue of that atomic operation
    # on uint8 is not supported by cuda
    packed = cp.zeros(int(num_bytes), dtype=cp.uint32)   
    pack_data_cuda[blockspergrid, threadsperblock](packed, values, positions, lengths)
    packed = packed.astype(cp.uint8)

    return packed



if __name__ == "__main__":
    # prepar data
    test_size = 1080 * 960 * 3
    values = 31*cp.ones(test_size).astype(cp.uint8)
    lengths = cp.full(test_size, 5, dtype=cp.uint8)


    packed = pack_data_cuda_wrapper(values, lengths)

    test_times = 1000
    t1 = time.time()
    for i in range(test_times):
        packed = pack_data_cuda_wrapper(values, lengths)
    t2 = time.time()
    
    print("FPS:",test_times/(t2 - t1))

    # # print result bytes
    packed_bin = ''.join(f'{byte:08b}' for byte in packed[0:10])
    print("Packed data:", packed_bin)



