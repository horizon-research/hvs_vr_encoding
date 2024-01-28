import cupy as cp
from numba import cuda
import time


@cuda.jit
def unpack_bits_to_uint8s_cuda(packed_data, bit_lengths, start_indices, end_indices, output):
    # Determine the index of the current thread
    i = cuda.grid(1)
    
    # Check if the thread index is within bounds
    if i < bit_lengths.size:
        int_value = 0
        start = start_indices[i]
        end = end_indices[i]
        for bit_idx in range(start, end):
            byte_index = bit_idx // 8
            bit_index = 7 - (bit_idx % 8)
            bit = (packed_data[byte_index] >> bit_index) & 1
            int_value = (int_value << 1) | bit
        output[i] = int_value

def unpack_bits_to_uint8s_cuda_wrapper(packed_data, bit_lengths):
    end_indices = cp.cumsum(bit_lengths).astype(cp.int32)
    start_indices = cp.zeros_like(end_indices)
    start_indices[1:] = end_indices[:-1]
    output = cp.zeros(len(bit_lengths), dtype=cp.uint8)

    # Calculate grid size
    threadsperblock = 32
    blockspergrid = (bit_lengths.size + (threadsperblock - 1)) // threadsperblock

    # Launch kernel
    unpack_bits_to_uint8s_cuda[blockspergrid, threadsperblock](packed_data, bit_lengths, start_indices, end_indices, output)

    return output


@cuda.jit
def unpack_bits_to_int8s_cuda(packed_data, bit_lengths, start_indices, end_indices, output):
    # Determine the index of the current thread
    i = cuda.grid(1)
    
    # Check if the thread index is within bounds
    if i < bit_lengths.size:
        int_value = 0
        neg = False
        start = start_indices[i]
        end = end_indices[i]
        for bit_idx in range(start, end):
            byte_index = bit_idx // 8
            bit_index = 7 - (bit_idx % 8)
            bit = (packed_data[byte_index] >> bit_index) & 1
            int_value = (int_value << 1) | bit
            if bit_idx == start:
                neg = (bit == 1)
        if neg:
            bitnum = end - start
            int_value = int_value - (1 << bitnum)
        output[i] = cp.int8(int_value).view(cp.uint8)

def unpack_bits_to_int8s_cuda_wrapper(packed_data, bit_lengths):
    end_indices = cp.cumsum(bit_lengths).astype(cp.int32)
    start_indices = cp.zeros_like(end_indices)
    start_indices[1:] = end_indices[:-1]
    output = cp.zeros(len(bit_lengths), dtype=cp.uint8)

    # Calculate grid size
    threadsperblock = 32
    blockspergrid = (bit_lengths.size + (threadsperblock - 1)) // threadsperblock

    # Launch kernel
    unpack_bits_to_int8s_cuda[blockspergrid, threadsperblock](packed_data, bit_lengths, start_indices, end_indices, output)

    return output

if __name__ == "__main__":
    # Test case
    # packed_data = np.array([0b11011010, 0b01101001], dtype=np.uint8)  # Example packed data
    # packed_data = np.array([218, 105], dtype=np.uint8)
    # bit_lengths = np.array([3, 3, 2, 6], dtype=np.uint8)  # Example lengths
    test_size = int(1080 * 960 * 3 / 2)
    test_times = 1000
    packed_data = cp.ones(test_size, dtype=cp.uint8) * 0b11110000
    bit_lengths = cp.ones( test_size * 2, dtype=cp.uint8) * 4
    unpack_bits_to_int8s_cuda_wrapper(packed_data, bit_lengths)
    t1 = time.time()
    for i in range(test_times):
        unpacked_data = unpack_bits_to_int8s_cuda_wrapper(packed_data, bit_lengths)
    t2 = time.time()
    print("FPS:",test_times/(t2 - t1))
    print("Unpacked Data:", unpacked_data[0:10])
