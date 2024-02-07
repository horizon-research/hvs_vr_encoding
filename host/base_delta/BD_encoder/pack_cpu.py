import numpy as np
import numba
import time

from numba import types

@numba.jit(nopython=True)
def pack_data_numba_wrapper(values: types.Array(types.uint8, 1, 'C'), lengths: types.Array(types.uint8, 1, 'C')):
    # Create an array to store the start position of each value
    positions = np.zeros(len(lengths), dtype=np.uint32)
    # Manually calculate the cumulative sum
    cum_sum = np.uint32(0)
    for i in range(len(lengths)):
        positions[i] = cum_sum
        cum_sum += lengths[i]

    # Calculate the total number of bits
    total_bits = cum_sum
    # Calculate the required number of bytes
    num_bytes = (total_bits + 7) // 8
    # Create an empty byte array
    packed = np.zeros(num_bytes, dtype=np.uint8)

    for i in range(len(values)):
        value = values[i]
        length = lengths[i]
        bit_pos = positions[i]
        for j in range(length):
            # Set the bits of the value at the correct position
            if value & (1 << j):
                byte_index = bit_pos // 8
                bit_index = bit_pos % 8
                packed[byte_index] |= 1 << bit_index
            bit_pos += 1
    return packed


if __name__ == "__main__":
    # Example data
    test_size = 1080 * 960 * 3
    values = 31 * np.ones((test_size)).astype(np.uint8) # 100 uint8 with value 31
    lengths = np.full(test_size, 5)  # All numbers use 5 bits


    test_times = 10
    t1 = time.time()
    for i in range(test_times):
        packed_data = pack_data_numba_wrapper(values, lengths)
    t2 = time.time()

    print("FPS:",test_times/(t2 - t1))

    # Convert the NumPy array to a binary string
    packed_bin = ''.join(f'{byte:08b}' for byte in packed_data[0:10])
    print("Packed data:", packed_bin)
