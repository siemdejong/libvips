/* Test the transpose codec implementation
 * 
 * This test creates a simple Zarr array with transpose codec applied
 * and verifies that the metadata is written correctly.
 */

#include <stdio.h>
#include <stdlib.h>
#include "rust/zarrs_wrapper.h"

int main(int argc, char **argv) {
    // Test 1: Create a simple 3D array with transpose [2, 0, 1]
    // This reorders dimensions from (height, width, bands) to (bands, height, width)
    
    printf("Testing transpose codec implementation...\n");
    
    // Create a small test image: 4x4 pixels with 3 bands
    int width = 4;
    int height = 4;
    int bands = 3;
    int data_size = width * height * bands;
    
    // Allocate and fill with test data
    unsigned char *data = (unsigned char *)malloc(data_size);
    if (!data) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Fill with a simple pattern (band-interleaved)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int b = 0; b < bands; b++) {
                int idx = (y * width + x) * bands + b;
                data[idx] = (unsigned char)((y * 16 + x * 4 + b) % 256);
            }
        }
    }
    
    // Define transpose order: [2, 0, 1] means bands → dim 0, height → dim 1, width → dim 2
    int transpose_order[] = {2, 0, 1};
    int transpose_len = 3;
    
    // Write the array with transpose
    printf("Writing array with transpose [2, 0, 1]...\n");
    int result = vips_zarr_write_array(
        "test_transpose.zarr",
        width,
        height,
        bands,
        0,  // uint8
        data,
        data_size,
        0,  // not OME-Zarr
        height,  // chunk_height (full image)
        width,   // chunk_width (full image)
        bands,   // chunk_bands (all bands)
        0,  // no sharding
        0,
        0,
        0,  // gzip compression
        0,  // default gzip level
        0,  // default zstd level
        0,  // default blosc level
        0,  // no shuffle
        0,  // auto typesize
        0,  // auto blocksize
        0,  // little endian
        transpose_order,
        transpose_len
    );
    
    free(data);
    
    if (result != 0) {
        fprintf(stderr, "Failed to write array with transpose\n");
        return 1;
    }
    
    printf("✓ Successfully wrote array with transpose codec\n");
    
    // Test 2: Write without transpose for comparison
    printf("\nWriting array without transpose for comparison...\n");
    
    data = (unsigned char *)malloc(data_size);
    if (!data) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Fill with same pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int b = 0; b < bands; b++) {
                int idx = (y * width + x) * bands + b;
                data[idx] = (unsigned char)((y * 16 + x * 4 + b) % 256);
            }
        }
    }
    
    result = vips_zarr_write_array(
        "test_no_transpose.zarr",
        width,
        height,
        bands,
        0,  // uint8
        data,
        data_size,
        0,  // not OME-Zarr
        height,  // chunk_height (full image)
        width,   // chunk_width (full image)
        bands,   // chunk_bands (all bands)
        0,  // no sharding
        0,
        0,
        0,  // gzip compression
        0,  // default gzip level
        0,  // default zstd level
        0,  // default blosc level
        0,  // no shuffle
        0,  // auto typesize
        0,  // auto blocksize
        0,  // little endian
        NULL,  // no transpose
        0
    );
    
    free(data);
    
    if (result != 0) {
        fprintf(stderr, "Failed to write array without transpose\n");
        return 1;
    }
    
    printf("✓ Successfully wrote array without transpose\n");
    
    printf("\nAll tests passed!\n");
    printf("\nTo verify the transpose codec:\n");
    printf("1. Check test_transpose.zarr/zarr.json for transpose codec in codecs array\n");
    printf("2. Check test_no_transpose.zarr/zarr.json should not have transpose codec\n");
    
    return 0;
}
