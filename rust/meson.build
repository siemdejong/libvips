# Build the Rust zarrs wrapper library using Cargo
# Meson's native Rust support doesn't handle Cargo dependencies,
# so we use Cargo directly and link the result

cargo = find_program('cargo', required: true)

# Collect Rust source files to track dependencies
rust_sources = files(
    'zarrs_wrapper/src/lib.rs',
    'zarrs_wrapper/src/ome_zarr.rs',
    'zarrs_wrapper/Cargo.toml',
    'zarrs_wrapper/Cargo.lock',
)

# Build the Rust library with Cargo
# We use a simple wrapper that builds and copies the output
zarrs_lib = custom_target('zarrs_wrapper',
    output: 'libzarrs_wrapper.a',
    input: rust_sources,
    command: [
        'sh', '-c',
        cargo.full_path() + ' build --release --manifest-path ' + meson.project_source_root() + '/rust/zarrs_wrapper/Cargo.toml --target-dir ' + meson.project_build_root() + '/rust/target && ' +
        'cp ' + meson.project_build_root() + '/rust/target/release/libzarrs_wrapper.a @OUTPUT@'
    ],
    build_by_default: true,
    console: true,
    depend_files: rust_sources,
)

# Declare the dependency using the actual output file
zarrs_wrapper_dep = declare_dependency(
    link_with: zarrs_lib,
    link_args: [
        '-lpthread',
        '-ldl',
        '-lm',
    ],
    include_directories: include_directories('.'),
)
