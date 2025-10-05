# Build the Rust zarrs wrapper library using Cargo
# Meson's native Rust support doesn't handle Cargo dependencies,
# so we use Cargo directly and link the result

cargo = find_program('cargo', required: true)

# Build the Rust library with Cargo
# We use a simple wrapper that builds and copies the output
zarrs_lib = custom_target('zarrs_wrapper',
    output: 'libzarrs_wrapper.a',
    input: 'zarrs_wrapper/Cargo.toml',
    command: [
        'sh', '-c',
        cargo.full_path() + ' build --release --manifest-path @INPUT@ --target-dir @BUILD_ROOT@/rust/target && ' +
        'cp @BUILD_ROOT@/rust/target/release/libzarrs_wrapper.a @OUTPUT@'
    ],
    build_by_default: true,
    console: true,
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
