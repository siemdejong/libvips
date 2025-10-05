# Zarrs Integration

This directory contains the Rust FFI wrapper for the [zarrs](https://github.com/zarrs/zarrs) library, allowing libvips (C/C++) to use zarrs functionality.

## Overview

The `zarrs` library is a Rust implementation of the Zarr storage format for multidimensional arrays. Since it's written in Rust, we use Foreign Function Interface (FFI) to make it accessible from C/C++ code.

## Structure

- `zarrs_wrapper/` - Rust crate that wraps zarrs
  - `Cargo.toml` - Rust package configuration with zarrs dependency
  - `src/lib.rs` - FFI wrapper functions with C-compatible ABI
- `zarrs_wrapper.h` - C header file declaring the exported functions
- `meson.build` - Meson build configuration for compiling the Rust library

## Available Functions

The following C-compatible functions are exported:

- `int vips_zarr_available(void)` - Check if zarrs is available (returns 1)
- `const char *vips_zarr_version(void)` - Get zarrs version string
- `int vips_zarr_test(void)` - Test zarrs functionality (returns 1 on success)

## Building

The Rust wrapper is automatically built as part of the libvips build process:

```bash
meson setup build
cd build
meson compile
```

### Requirements

- Rust compiler (rustc) and Cargo
- All zarrs dependencies (automatically fetched by Cargo)

## How It Works

1. **Rust Code**: The `zarrs_wrapper` crate depends on `zarrs 0.22` and exposes C-compatible functions using `extern "C"` and `#[no_mangle]`

2. **Build Process**: During the meson build, `cargo build --release` is invoked to compile the Rust code into a static library (`libzarrs_wrapper.a`)

3. **Linking**: The static library is linked into libvips using `--whole-archive` to ensure all symbols are included, along with required system libraries (`pthread`, `dl`)

4. **Usage**: C/C++ code can include `rust/zarrs_wrapper.h` and call the exported functions directly

## Extending

To add more zarrs functionality:

1. Add new functions to `zarrs_wrapper/src/lib.rs` with `#[no_mangle] pub extern "C"` attributes
2. Declare the functions in `zarrs_wrapper.h`
3. Use the functions from libvips C/C++ code

Example:

```rust
// In src/lib.rs
#[no_mangle]
pub extern "C" fn vips_zarr_open(path: *const c_char) -> i32 {
    // Implementation using zarrs
}
```

```c
// In zarrs_wrapper.h
int vips_zarr_open(const char *path);
```

## Downstream Compatibility

Downstream packages (pyvips, php-vips, etc.) work without modification because:

- The Rust code is statically linked into libvips
- Only the C API is exposed
- No Rust runtime is required at runtime
- The integration is transparent to users

## Notes

- The zarrs library and all its dependencies are statically linked
- No additional runtime dependencies beyond standard system libraries (pthread, dl)
- The Rust code is compiled in release mode for optimal performance
