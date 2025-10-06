#include <stdio.h>
#include <stdlib.h>
#include "rust/zarrs_wrapper.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <zarr_path>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    printf("Attempting to open: %s\n", path);

    VipsZarrReadHandle handle = vips_zarr_open(path);
    if (!handle) {
        fprintf(stderr, "Failed to open zarr array\n");
        return 1;
    }

    uint64_t width, height, bands;
    int data_type;
    
    if (vips_zarr_get_metadata(handle, &width, &height, &bands, &data_type) < 0) {
        fprintf(stderr, "Failed to get metadata\n");
        vips_zarr_close(handle);
        return 1;
    }

    printf("Success! Dimensions: %lu x %lu x %lu, data_type: %d\n",
           (unsigned long)width, (unsigned long)height, (unsigned long)bands, data_type);

    vips_zarr_close(handle);
    return 0;
}
