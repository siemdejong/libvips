/* zarrs_wrapper.h - C header for Rust zarrs FFI wrapper
 */

#ifndef VIPS_ZARRS_WRAPPER_H
#define VIPS_ZARRS_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if zarrs is available and working
 * Returns 1 if zarrs is available, 0 otherwise
 */
int vips_zarr_available(void);

/* Get the zarrs version string
 * Returns a pointer to a static string containing the version
 */
const char *vips_zarr_version(void);

/* Simple test function to verify zarrs functionality
 * Returns 1 on success, 0 on failure
 */
int vips_zarr_test(void);

/* Create a new Zarr v3 array and write data to it
 * 
 * Arguments:
 *   path - Path to the zarr store directory (null-terminated C string)
 *   width - Width of the image
 *   height - Height of the image  
 *   bands - Number of bands/channels
 *   data_type - Data type:
 *       0=uint8, 1=uint16, 2=uint32, 3=float32, 4=float64
 *       5=int8, 6=int16, 7=int32, 8=uint64, 9=int64
 *       10=complex64, 11=complex128
 *   data - Pointer to the image data
 *   data_len - Length of data in bytes
 *   ome_zarr - If 1, write OME-Zarr compatible metadata
 *   chunk_height - Chunk height (0 for full image height)
 *   chunk_width - Chunk width (0 for full image width)
 *   chunk_bands - Chunk bands (0 for all bands)
 *   shard_height - Shard height (0 for no sharding)
 *   shard_width - Shard width (0 for no sharding)
 *   shard_bands - Shard bands (0 for no sharding)
 *   compression - Compression codec: 0=gzip, 1=zstd, 2-7=blosc variants
 *   gzip_level - Gzip compression level (1-9, 0 for default of 5)
 *   zstd_level - Zstd compression level (1-22, 0 for default of 3)
 *   blosc_clevel - Blosc compression level (0-9, 0 for default of 5)
 *   blosc_shuffle - Blosc shuffle mode (0=noshuffle, 1=shuffle, 2=bitshuffle)
 *   blosc_typesize - Blosc typesize (0 for automatic based on data type)
 *   blosc_blocksize - Blosc blocksize in bytes (0 for automatic)
 * 
 * Returns:
 *   0 on success, -1 on error
 */
int vips_zarr_write_array(
    const char *path,
    uint64_t width,
    uint64_t height,
    uint64_t bands,
    int32_t data_type,
    const uint8_t *data,
    size_t data_len,
    int ome_zarr,
    int chunk_height,
    int chunk_width,
    int chunk_bands,
    int shard_height,
    int shard_width,
    int shard_bands,
    int compression,
    int gzip_level,
    int zstd_level,
    int blosc_clevel,
    int blosc_shuffle,
    int blosc_typesize,
    int blosc_blocksize
);

#ifdef __cplusplus
}
#endif

#endif /* VIPS_ZARRS_WRAPPER_H */
