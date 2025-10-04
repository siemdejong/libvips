project('tensorstore', 'cpp', 'c')

message('=== Using custom TensorStore Meson build ===')

# Your custom build logic here
protobuf_dep = dependency('protobuf', required: true)
protoc_exe = find_program('protoc', required: true)

# Build TensorStore components you need
tensorstore_dep = declare_dependency(
    # Your dependency configuration
)

# Make it available to parent project
