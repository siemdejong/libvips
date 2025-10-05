# Zarr v3 Support in libvips

This document describes the integration of Zarr v3 format support into libvips using the Rust `zarrs` library.

## Overview

libvips now supports saving images in Zarr v3 format through a new `zarrsave` operation. The implementation uses a Rust FFI wrapper around the `zarrs` library, linked into the main libvips shared library.

## Architecture

### Components

1. **Rust FFI Wrapper** (`rust/zarrs_wrapper/`)
   - Provides C-compatible interface to the Rust zarrs library
   - Static library (`libzarrs_wrapper.a`) linked into libvips
   - Dependencies: `zarrs = "0.22"`, `zarrs_filesystem = "0.3"`

2. **C Integration** (`libvips/foreign/zarrsave.c`)
   - Implements `VipsForeignSaveZarr` class
   - Registers `.zarr` file suffix
   - Converts VIPS image formats to Zarr data types

3. **Build System** (`meson.build`, `rust/meson.build`)
   - Cargo integration for Rust library compilation
   - Proper linking order to include Rust dependencies

## Supported Features

### Data Types
- `uint8` (VIPS_FORMAT_UCHAR)
- `uint16` (VIPS_FORMAT_USHORT)
- `uint32` (VIPS_FORMAT_UINT)
- `float32` (VIPS_FORMAT_FLOAT)
- `float64` (VIPS_FORMAT_DOUBLE)

### Image Properties
- Any number of bands (grayscale, RGB, RGBA, etc.)
- Any image dimensions
- Automatic compression using gzip (level 5)

### Zarr v3 Format
- Metadata stored in `zarr.json`
- Chunks stored in `c/x/y/z` directory structure  
- Gzip compression applied to chunks
- Optimization: chunks containing only fill values are not written

### OME-NGFF Support
- Optional OME-NGFF (Open Microscopy Environment - Next Generation File Format) v0.5 compatible output
- Proper `multiscales` metadata with `axes`, `datasets`, and `coordinateTransformations`
- Automatic axis detection (channel, spatial y/x)
- Array stored in subdirectory (`0/`) per OME-NGFF specification
- Physical units (micrometers) for spatial axes
- Compatible with OME-Zarr readers and visualization tools

## Usage

### Basic Example
```bash
# Save an image to Zarr format
vips copy input.jpg output.zarr

# Create and save a test image
vips black test.v 256 256 --bands 3
vips invert test.v test_white.v
vips copy test_white.v test.zarr
```

### OME-NGFF Example
```bash
# Save with OME-NGFF metadata
vips copy input.tif output.zarr[ome_ngff]

# Or explicitly set the option
vips copy input.tif output.zarr[ome_ngff=true]

# Regular Zarr without OME metadata
vips copy input.tif output.zarr[ome_ngff=false]
```

### Verify Output
```bash
# Check metadata
cat output.zarr/zarr.json

# For regular Zarr
find output.zarr/c -type f

# For OME-NGFF Zarr
find output.zarr/0/c -type f

# Verify chunk compression
file output.zarr/c/0/0/0  # regular
file output.zarr/0/c/0/0/0  # OME-NGFF

# View OME-NGFF metadata
cat output.zarr/zarr.json | grep -A 30 '"ome"'
```

## Implementation Details

### Array Shape
Images are stored with shape `[height, width, bands]` to match VIPS' memory layout:
- Single-band: `[H, W, 1]`
- RGB: `[H, W, 3]`
- RGBA: `[H, W, 4]`

### OME-NGFF Metadata Structure
When `ome_ngff=true`, the output conforms to OME-NGFF v0.5:

```json
{
  "zarr_format": 3,
  "node_type": "group",
  "attributes": {
    "ome": {
      "version": "0.5",
      "multiscales": [{
        "version": "0.5",
        "name": "libvips-zarr",
        "axes": [
          {"name": "c", "type": "channel"},  // Only if bands > 1
          {"name": "y", "type": "space", "unit": "micrometer"},
          {"name": "x", "type": "space", "unit": "micrometer"}
        ],
        "datasets": [{
          "path": "0",
          "coordinateTransformations": [{
            "type": "scale",
            "scale": [1.0, 1.0, 1.0]  // [c, y, x] or [y, x]
          }]
        }]
      }]
    }
  }
}
```

**Key differences from regular Zarr:**
- Array stored in `0/` subdirectory instead of root
- Group-level metadata with OME namespace
- Axes definitions with types and units
- Coordinate transformations (scale)
- Compatible with OME-Zarr viewers and analysis tools

### Chunking Strategy
Currently uses the entire image as a single chunk for simplicity:
- Chunk shape: `[height, width, bands]`
- Future enhancement: configurable chunk sizes for better performance with large images

### Fill Values
Fill values match the data type:
- Integer types: `0`
- Float types: `0.0`

### Memory Management
- Full image loaded into memory before writing
- Array explicitly dropped to ensure flush to disk
- No streaming support (limitation of current implementation)

## Testing

Run the test script:
```bash
./test_zarrsave.sh
```

This tests:
1. Single-band uint8 images
2. Multi-band (RGB) uint8 images
3. Single-band float32 images

## Build Instructions

### Prerequisites
- Rust toolchain (cargo)
- Meson ≥ 1.9.1
- Standard libvips build dependencies

### Build Steps
```bash
# Configure
meson setup build

# Build (Rust library is built automatically)
cd build
ninja

# Note: If Rust library doesn't copy correctly, manual step:
cp rust/target/release/libzarrs_wrapper.a rust/
```

### Build System Details
The Rust library is built as part of the Meson build:
1. `rust/meson.build` defines a custom_target that runs `cargo build --release`
2. The static library is copied to the expected location
3. Main `meson.build` adds it to `external_deps` before `libvips_deps`
4. libvips links against the static library

## Known Limitations

1. **No streaming**: Entire image loaded into memory
2. **Fixed chunk size**: Uses full image as one chunk
3. **No chunk cache**: Future optimization opportunity
4. **Gzip only**: No alternative codecs (e.g., blosc, zstd)
5. **No metadata**: Custom VIPS metadata not preserved
6. **Single resolution**: OME-NGFF output only includes one resolution level

## Future Enhancements

- [ ] Configurable chunk sizes
- [ ] Additional compression codecs
- [ ] Streaming write support for large images
- [ ] Preserve VIPS metadata
- [ ] Read support (zarrload)
- [ ] Support for Zarr v2 format
- [ ] Parallel chunk writing
- [ ] Multi-resolution pyramids for OME-NGFF
- [ ] Labels support for OME-NGFF segmentation data

## Files Modified/Added

### New Files
- `rust/zarrs_wrapper/Cargo.toml` - Rust project configuration (with serde_json dependency)
- `rust/zarrs_wrapper/src/lib.rs` - FFI implementation with OME-NGFF support
- `rust/zarrs_wrapper/src/ome_ngff.rs` - OME-NGFF metadata generation
- `rust/zarrs_wrapper.h` - C header for FFI functions
- `rust/meson.build` - Rust build integration
- `libvips/foreign/zarrsave.c` - VipsForeignSave implementation with ome_ngff option
- `test_zarrsave.sh` - Test script

### Modified Files
- `meson.build` - Added zarrs_wrapper_dep to external_deps
- `libvips/foreign/meson.build` - Added zarrsave.c to sources
- `libvips/foreign/foreign.c` - Registered zarrsave operation

## References

- Zarr v3 Specification: https://zarr-specs.readthedocs.io/en/latest/v3/core/v3.0.html
- OME-NGFF Specification v0.5: https://ngff.openmicroscopy.org/0.5/
- zarrs Rust library: https://github.com/zarrs/zarrs
- libvips: https://www.libvips.org/

## License

This integration follows the same license as libvips (LGPL 2.1+).
