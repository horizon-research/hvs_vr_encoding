import numpy as np
import numba
import time
from numba import types

@numba.jit(nopython=True)
def unpack_bits_to_uint8s(packed_data: types.Array(types.uint8, 1, 'C'), bit_lengths: types.Array(types.uint8, 1, 'C')):
    """
    Manually unpack bits from packed_data into integers based on specified bit_lengths.
    """
    bit_lengths = bit_lengths.astype(np.uint32)
    end_indices = np.cumsum(bit_lengths)
    start_indices = np.zeros_like(end_indices)
    start_indices[1:] = end_indices[:-1]

    ints = np.zeros(len(bit_lengths), dtype=np.uint8)
    for i, (start, end) in enumerate(zip(start_indices, end_indices)):
        int_value = 0
        for bit_idx in range(start, end):
            byte_index = bit_idx // 8
            bit_index = (bit_idx % 8)
            bit = (packed_data[byte_index] >> bit_index) & 1
            int_value = int_value | (bit << (bit_idx - start))
        ints[i] = np.uint8(int_value)

    return ints

@numba.jit(nopython=True)
def unpack_bits_to_int8s(packed_data, bit_lengths):
    """
    Manually unpack bits from packed_data into integers based on specified bit_lengths.
    """
    bit_lengths = bit_lengths.astype(np.uint32)
    end_indices = np.cumsum(bit_lengths)
    start_indices = np.zeros_like(end_indices)
    start_indices[1:] = end_indices[:-1]

    ints = np.zeros(len(bit_lengths), dtype=np.uint8)
    for i, (start, end) in enumerate(zip(start_indices, end_indices)):
        int_value = 0
        neg = False
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
        ints[i] = np.int8(int_value).view(np.uint8)

    return ints



if __name__ == "__main__":
    # Test case
    # packed_data = np.array([0b11011010, 0b01101001], dtype=np.uint8)  # Example packed data
    # packed_data = np.array([218, 105], dtype=np.uint8)
    # bit_lengths = np.array([3, 3, 2, 6], dtype=np.uint8)  # Example lengths
    test_size = int(1080 * 960 * 3 / 2)
    test_times = 100
    packed_data = np.ones(test_size, dtype=np.uint8) * 0b11110000
    bit_lengths = np.ones( test_size * 2, dtype=np.uint8) * 4
    unpack_bits_to_int8s(packed_data, bit_lengths)
    t1 = time.time()
    for i in range(test_times):
        unpacked_data = unpack_bits_to_int8s(packed_data, bit_lengths)
    t2 = time.time()
    print("FPS:",test_times/(t2 - t1))
    print("Unpacked Data:", unpacked_data[0:200])
