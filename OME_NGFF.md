# OME-Zarr Support in libvips zarrsave

## Overview

libvips now supports writing OME-Zarr (Open Microscopy Environment - Next Generation File Format) v0.5 compatible Zarr arrays. This allows microscopy images to be stored in a format that is widely supported by the bioimaging community.

## Quick Start

```bash
# Regular Zarr v3
vips copy input.tif output.zarr

# OME-Zarr Zarr v3
vips copy input.tif output.zarr[ome_zarr]
```

## What is OME-Zarr?

OME-Zarr is a specification for storing multi-dimensional bioimaging data using the Zarr format. It adds standardized metadata that describes:

- **Axes**: Dimensions and their types (spatial, channel, time)
- **Physical units**: Real-world measurements (micrometers, seconds, etc.)
- **Coordinate transformations**: How data coordinates map to physical space
- **Multiscale pyramids**: Multiple resolution levels for large images

## Metadata Structure

### Regular Zarr
```
output.zarr/
├── zarr.json          # Simple array metadata
└── c/0/0/0           # Chunk data at root
```

### OME-Zarr Zarr
```
output.zarr/
├── zarr.json          # Group metadata with OME namespace
└── 0/                 # Resolution level
    ├── zarr.json      # Array metadata
    └── c/0/0/0       # Chunk data
```

### Example OME Metadata

**3-band RGB image:**
```json
{
  "ome": {
    "version": "0.5",
    "multiscales": [{
      "name": "libvips-zarr",
      "axes": [
        {"name": "c", "type": "channel"},
        {"name": "y", "type": "space", "unit": "micrometer"},
        {"name": "x", "type": "space", "unit": "micrometer"}
      ],
      "datasets": [{
        "path": "0",
        "coordinateTransformations": [{
          "type": "scale",
          "scale": [1.0, 1.0, 1.0]
        }]
      }]
    }]
  }
}
```

**1-band grayscale image:**
```json
{
  "axes": [
    {"name": "y", "type": "space", "unit": "micrometer"},
    {"name": "x", "type": "space", "unit": "micrometer"}
  ],
  "datasets": [{
    "coordinateTransformations": [{
      "type": "scale",
      "scale": [1.0, 1.0]
    }]
  }]
}
```

## Key Features

1. **Automatic axis detection**: Channel axis only added for multi-band images
2. **Zarr v3 compliant**: Uses latest Zarr specification
3. **OME-Zarr v0.5**: Compatible with latest spec version
4. **Physical units**: Spatial axes use micrometers by default
5. **Single resolution**: Currently supports one resolution level

## Use Cases

### Microscopy Data
```bash
# Convert microscopy TIFF to OME-Zarr
vips copy microscopy.tif microscopy.zarr[ome_zarr]
```

### High-Content Screening
```bash
# Convert well plate images
for well in A01 A02 A03; do
    vips copy plate_${well}.tif plate.zarr/${well}[ome_zarr]
done
```

### Remote Data Access
OME-Zarr format enables:
- Cloud-native storage (S3, GCS)
- Chunk-based streaming
- Partial image loading
- Multi-resolution visualization

## Compatibility

### Readers
OME-Zarr output can be read by:
- [napari](https://napari.org/) - Multi-dimensional image viewer
- [neuroglancer](https://github.com/google/neuroglancer) - WebGL visualization
- [OMERO](https://www.openmicroscopy.org/omero/) - Image data management
- [QuPath](https://qupath.github.io/) - Pathology image analysis
- [zarr-python](https://zarr.readthedocs.io/) - Python Zarr library
- [ome-zarr-py](https://github.com/ome/ome-zarr-py) - OME-Zarr Python tools

### Validation
```bash
# Install ome-zarr-py
pip install ome-zarr

# Validate OME-Zarr output
ome_zarr info output.zarr

# View in napari
napari output.zarr
```

## Implementation Details

### Rust Module
- `src/ome_zarr.rs`: Metadata generation
- Implements OME-Zarr v0.5 specification
- JSON generation using serde_json
- Automatic axis ordering (time > channel > space)

### C Interface
- `ome_zarr` boolean option in VipsForeignSaveZarr
- Passed through FFI to Rust implementation
- No performance overhead when disabled

## Limitations

- Single resolution (no pyramids yet)
- No label images support
- No plate/well metadata
- Physical pixel size defaults to 1.0 micrometer
- No time axis support

## Future Enhancements

- Multi-resolution pyramids
- Custom physical pixel sizes
- Time-series support (5D arrays)
- Label images for segmentation
- HCS (High-Content Screening) plate metadata
- OME-XML preservation

## References

- [OME-Zarr Specification](https://ngff.openmicroscopy.org/0.5/)
- [Zarr v3 Specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/v3.0.html)
- [OME-Zarr GitHub](https://github.com/ome/ome-zarr-py)
- [NGFF Tools](https://ngff.openmicroscopy.org/tools/)

## License

LGPL 2.1+ (same as libvips)
