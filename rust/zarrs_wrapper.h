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
 *   data_type - Data type: 0=uint8, 1=uint16, 2=uint32, 3=float32, 4=float64
 *   data - Pointer to the image data
 *   data_len - Length of data in bytes
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
    size_t data_len
);

#ifdef __cplusplus
}
#endif

#endif /* VIPS_ZARRS_WRAPPER_H */
