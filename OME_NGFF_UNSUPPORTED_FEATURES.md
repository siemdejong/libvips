# OME-NGFF (OME-Zarr) Specification Compliance - Unsupported Features

This document lists all features from the [OME-NGFF v0.5 Specification](https://ngff.openmicroscopy.org/0.5/) that are **not yet supported** by the libvips Zarr implementation.

**Last Updated:** October 5, 2025  
**OME-NGFF Spec Version:** 0.5 (September 25, 2025)  
**libvips Branch:** zarr

---

## Current Implementation Summary

The libvips OME-NGFF implementation is **minimal** and focuses on basic single-resolution image writing:

**Supported:**
- ✅ Basic `multiscales` metadata with single resolution level
- ✅ Simple `axes` definitions (c, y, x only)
- ✅ Basic `coordinateTransformations` (scale only, hardcoded to 1.0)
- ✅ Zarr v3 storage format
- ✅ Group-level metadata in `zarr.json`

**Not Supported:**
- ❌ Multi-resolution pyramids
- ❌ Labels (segmentation)
- ❌ High-content screening (plates/wells)
- ❌ Most metadata features
- ❌ Time and Z dimensions
- ❌ Custom transformations
- ❌ And much more...

---

## 1. Storage Format Features

### 1.1. Image Layout - Not Supported

- ❌ **Multi-resolution pyramids** - Only single resolution (`/0`) is created
- ❌ Multiple resolution levels (`/0`, `/1`, `/2`, etc.)
- ❌ Resolution level names beyond `/0`
- ❌ Intermediate groups between root and arrays
- ❌ Custom array naming schemes

### 1.2. Labels - Not Supported

- ❌ **Entire labels feature** - No label/segmentation support
- ❌ `labels/` group creation
- ❌ Label images (segmentation masks)
- ❌ Integer-only arrays for labels
- ❌ Multiple label images per original image
- ❌ Intermediate folders in labels hierarchy
- ❌ `labels` array in group metadata
- ❌ Label discovery mechanism

### 1.3. High-Content Screening (HCS) - Not Supported

- ❌ **Entire HCS feature set** - No plate/well support
- ❌ Plate layout
- ❌ Well groups
- ❌ Well rows
- ❌ Field of view (FOV) images
- ❌ Plate hierarchy (plate → row → well → image)
- ❌ Acquisition tracking
- ❌ Sparse plate support

---

## 2. OME-Zarr Metadata

### 2.1. "axes" Metadata

#### Supported
- ✅ Basic axes: `c` (channel), `y` (space), `x` (space)
- ✅ `name` field
- ✅ `type` field (space, channel)
- ✅ `unit` field (hardcoded to "micrometer" for spatial axes)

#### Not Supported
- ❌ **Time axis** (`t`) - No temporal dimension support
- ❌ **Z axis** (`z`) - No volumetric/depth dimension
- ❌ 4D data (t, z, y, x)
- ❌ 5D data (t, c, z, y, x)
- ❌ Custom axis types beyond space/channel/time
- ❌ Custom axis names
- ❌ Configurable units (currently hardcoded to "micrometer")
- ❌ All time units:
  - attosecond, centisecond, day, decisecond, exasecond, femtosecond
  - gigasecond, hectosecond, hour, kilosecond, megasecond, microsecond
  - millisecond, minute, nanosecond, petasecond, picosecond, second
  - terasecond, yoctosecond, yottasecond, zeptosecond, zettasecond
- ❌ Most space units (only "micrometer" supported):
  - angstrom, attometer, centimeter, decimeter, exameter, femtometer
  - foot, gigameter, hectometer, inch, kilometer, megameter, meter
  - mile, millimeter, nanometer, parsec, petameter, picometer
  - terameter, yard, yoctometer, yottameter, zeptometer, zettameter
- ❌ Unit-less axes
- ❌ Anisotropic axes (different scaling for z vs xy)
- ❌ `dimension_names` attribute in array `zarr.json` (REQUIRED by spec but not implemented)

### 2.2. "bioformats2raw.layout" (Transitional) - Not Supported

- ❌ **Entire bioformats2raw layout** - Not implemented
- ❌ `bioformats2raw.layout` key
- ❌ Multi-image collections
- ❌ Series metadata
- ❌ `OME/` special group
- ❌ `METADATA.ome.xml` file
- ❌ OME-XML metadata
- ❌ Series index mapping
- ❌ Numbered image groups (0/, 1/, 2/, ...)
- ❌ Mixed plate + collection layouts

### 2.3. "coordinateTransformations" Metadata

#### Supported
- ✅ `scale` transformation (hardcoded to 1.0 for all axes)

#### Not Supported
- ❌ **Custom scale values** - All scales hardcoded to 1.0
- ❌ `translation` transformation - No offset support
- ❌ `identity` transformation - Not explicitly used
- ❌ Path-based transformations (`"path": "..."`)
- ❌ Binary transformation data
- ❌ Multiple transformations in sequence
- ❌ Per-resolution transformations beyond scale
- ❌ Group-level coordinate transformations
- ❌ Physical pixel size specification
- ❌ Non-uniform scaling per axis
- ❌ Transformation order customization

### 2.4. "multiscales" Metadata

#### Supported
- ✅ Basic `multiscales` list with single entry
- ✅ `axes` field (limited to c, y, x)
- ✅ `datasets` field with single dataset
- ✅ `path` field ("0" only)
- ✅ `coordinateTransformations` per dataset (scale only)
- ✅ `name` field (hardcoded to "libvips-zarr")
- ✅ `type` field (hardcoded to "none")
- ✅ `metadata` field (minimal info)

#### Not Supported
- ❌ **Multiple resolution levels** - Only single resolution
- ❌ Multiple datasets in `datasets` array
- ❌ Image pyramids (downsampled versions)
- ❌ Multiple multiscale entries
- ❌ Custom dataset paths beyond "0"
- ❌ Downscaling method documentation
- ❌ Downscaling types:
  - "gaussian"
  - "average"
  - "mode"
  - "median"
  - Custom methods
- ❌ Downscaling metadata with method details
- ❌ Version tracking for downscaling algorithms
- ❌ Downscaling parameters (args, kwargs)
- ❌ Scale factor specification between levels
- ❌ Custom multiscale naming
- ❌ Multiple named multiscales with selection
- ❌ Chunk size optimization per resolution
- ❌ Group-level coordinate transformations
- ❌ Time/channel-specific transformations
- ❌ Anisotropic downsampling
- ❌ Z-stack specific handling (zyx order)

### 2.5. "omero" Metadata (Transitional) - Not Supported

- ❌ **Entire OMERO metadata** - Not implemented
- ❌ `omero` key
- ❌ Channel rendering information:
  - `active` - Channel visibility
  - `coefficient` - Intensity multiplier
  - `color` - RGB color string
  - `family` - Rendering family (linear, polynomial, etc.)
  - `inverted` - Invert intensity
  - `label` - Channel name
  - `window` - Display window (start, end, min, max)
- ❌ `rdefs` - Rendering defaults:
  - `defaultT` - Default timepoint
  - `defaultZ` - Default Z section
  - `model` - Color model (color/greyscale)
- ❌ `id` - OMERO image ID
- ❌ `name` - Display name
- ❌ OMERO WebGateway compatibility

### 2.6. "labels" Metadata - Not Supported

- ❌ **Entire labels specification** - No segmentation support
- ❌ `labels` group
- ❌ `labels` array in group metadata
- ❌ Label image arrays
- ❌ `image-label` metadata:
  - `colors` - Color mapping for label values
  - `label-value` - Integer label identifiers
  - `rgba` - RGBA color arrays
  - `properties` - Label properties/metadata
  - `source` - Source image reference
  - `version` - Label schema version
- ❌ Integer data types for labels (uint8/16/32/64, int8/16/32/64)
- ❌ Label discovery
- ❌ Multiple labels per image
- ❌ Intermediate label folders
- ❌ Label-to-image coordinate correspondence
- ❌ Custom label properties
- ❌ Label color display specifications

### 2.7. "plate" Metadata - Not Supported

- ❌ **Entire plate specification** - No HCS support
- ❌ `plate` key
- ❌ Plate layout definition:
  - `acquisitions` - List of acquisitions
  - `columns` - Column definitions
  - `rows` - Row definitions
  - `wells` - Well locations
  - `field_count` - Max fields per well
  - `name` - Plate name
  - `version` - Plate spec version
- ❌ Acquisition tracking:
  - `id` - Acquisition ID
  - `name` - Acquisition name
  - `maximumfieldcount` - Max FOVs
  - `description` - Acquisition description
  - `starttime` - Epoch timestamp
  - `endtime` - Epoch timestamp
- ❌ Row/column specifications:
  - `name` - Row/column identifier
  - Alphanumeric naming
  - Case-sensitive names
- ❌ Well specifications:
  - `path` - Well path (e.g., "A/1")
  - `rowIndex` - 0-based row index
  - `columnIndex` - 0-based column index
- ❌ Sparse plate support
- ❌ Full plate definitions (96-well, 384-well, etc.)
- ❌ Well row groups

### 2.8. "well" Metadata - Not Supported

- ❌ **Entire well specification** - No HCS support
- ❌ `well` key
- ❌ Field of view definitions:
  - `images` - List of FOV images
  - `path` - FOV path
  - `acquisition` - Acquisition ID reference
  - `version` - Well spec version
- ❌ Multiple FOVs per well
- ❌ Multiple acquisitions per well
- ❌ FOV naming schemes
- ❌ Acquisition-to-FOV mapping

---

## 3. Dimensions and Axes

### Currently Supported
- ✅ 2D images (y, x) - with implicit bands=1
- ✅ 3D images (c, y, x) - channel as first dimension

### Not Supported
- ❌ **Volumetric images** (z, y, x or c, z, y, x)
- ❌ **Time-series images** (t, y, x or t, c, y, x)
- ❌ **5D images** (t, c, z, y, x)
- ❌ 4D variants (t, z, y, x or t, c, y, x)
- ❌ Custom dimension ordering
- ❌ Dimension names beyond c, y, x
- ❌ Optional dimensions (selective inclusion of t, c, z)
- ❌ 1D data
- ❌ >5D data
- ❌ Planar storage (separate planes for each channel/z/t)

---

## 4. Coordinate Systems and Physical Units

### Currently Supported
- ✅ Spatial units: "micrometer" only (hardcoded)
- ✅ Scale transformations (hardcoded to 1.0)

### Not Supported
- ❌ **Actual physical pixel sizes** - All scales = 1.0
- ❌ Custom pixel sizes (nm, μm, mm, etc.)
- ❌ Anisotropic pixels (different x/y/z resolution)
- ❌ Time duration specification
- ❌ Translation/offset from origin
- ❌ Non-identity coordinate transformations
- ❌ Path-based transformation storage
- ❌ Unit conversion
- ❌ Physical coordinate queries
- ❌ ROI in physical coordinates
- ❌ All units except "micrometer":
  - Space: angstrom, attometer, centimeter, decimeter, exameter, femtometer, foot, gigameter, hectometer, inch, kilometer, megameter, meter, mile, millimeter, nanometer, parsec, petameter, picometer, terameter, yard, yoctometer, yottameter, zeptometer, zettameter
  - Time: attosecond, centisecond, day, decisecond, exasecond, femtosecond, gigasecond, hectosecond, hour, kilosecond, megasecond, microsecond, millisecond, minute, nanosecond, petasecond, picosecond, second, terasecond, yoctosecond, yottasecond, zeptosecond, zettasecond

---

## 5. Metadata Features

### Not Supported
- ❌ **Custom attributes** - No user-defined metadata
- ❌ Group attributes beyond OME metadata
- ❌ Array-level custom attributes
- ❌ Arbitrary JSON in attributes
- ❌ Nested metadata structures
- ❌ Metadata versioning beyond spec version
- ❌ Metadata schemas
- ❌ Metadata validation
- ❌ Metadata updates
- ❌ Extension metadata
- ❌ Application-specific metadata
- ❌ Provenance tracking
- ❌ Processing history
- ❌ Acquisition metadata beyond basics
- ❌ Sample metadata
- ❌ Instrument metadata
- ❌ Experimenter metadata

---

## 6. Image Pyramids (Multi-Resolution)

### Not Supported
- ❌ **Entire pyramid feature** - Single resolution only
- ❌ Multiple resolution levels
- ❌ Downsampling/decimation
- ❌ Resolution level generation
- ❌ Pyramid types:
  - Gaussian pyramid
  - Average pooling pyramid
  - Mode/median pyramids
  - Custom pyramids
- ❌ Scale factor specification
- ❌ Per-resolution chunk optimization
- ❌ Progressive loading
- ❌ Level-of-detail rendering
- ❌ Automatic level selection
- ❌ Pyramid validation

---

## 7. Visualization and Rendering

### Not Supported (OMERO metadata)
- ❌ **All rendering metadata** - No display hints
- ❌ Channel colors
- ❌ Channel visibility (active/inactive)
- ❌ Display windows (min/max/start/end)
- ❌ Color models (color vs greyscale)
- ❌ Intensity adjustments
- ❌ Default Z-section selection
- ❌ Default timepoint selection
- ❌ Channel names/labels
- ❌ LUT (lookup table) specifications
- ❌ Rendering families (linear, polynomial, etc.)
- ❌ Channel coefficients

---

## 8. High-Content Screening Features

### Not Supported
- ❌ **Entire HCS ecosystem** - No screening support
- ❌ Plate definitions
- ❌ Well definitions
- ❌ Field of view management
- ❌ Acquisition series
- ❌ Multiple timepoints per well
- ❌ Multiple FOVs per well
- ❌ Row/column layouts
- ❌ Sparse plates
- ❌ Plate naming
- ❌ Well indexing
- ❌ Acquisition timestamps
- ❌ Maximum field counts
- ❌ Plate-level metadata
- ❌ Well-level metadata
- ❌ Image-to-acquisition mapping

---

## 9. File Organization

### Supported
- ✅ Basic Zarr v3 group structure
- ✅ Single array at `/0`
- ✅ Root `zarr.json` with group metadata

### Not Supported
- ❌ **Multiple arrays in hierarchy**
- ❌ Nested groups beyond root
- ❌ Custom array paths
- ❌ Hierarchical organization (plates/wells/images)
- ❌ Labels subdirectory
- ❌ OME/ metadata directory
- ❌ Intermediate folders
- ❌ Collection layouts
- ❌ Series organization
- ❌ Mixed content (images + labels + metadata)

---

## 10. Data Types and Storage

### Supported
- ✅ Basic data types: uint8, uint16, uint32, float32, float64
- ✅ Interleaved channel storage (c, y, x order)

### Not Supported
- ❌ **Integer labels** - No int8/16/32/64 support for labels
- ❌ Label-specific data types (uint64, int* types)
- ❌ Planar channel storage
- ❌ Complex data types
- ❌ Custom data type extensions
- ❌ Data type per resolution level
- ❌ Mixed data types in pyramid

---

## 11. Reading and Roundtrip Support

### Not Supported
- ❌ **Reading OME-Zarr files** - Write-only implementation
- ❌ Roundtrip testing
- ❌ OME-Zarr validation
- ❌ Metadata parsing
- ❌ Multi-resolution reading
- ❌ Selective resolution loading
- ❌ Label reading
- ❌ Plate/well reading
- ❌ Metadata queries
- ❌ Coordinate transformation application

---

## 12. Interoperability

### Not Supported
- ❌ **bioformats2raw compatibility** - No collection support
- ❌ OMERO compatibility - No omero metadata
- ❌ OME-XML generation
- ❌ OME-TIFF relationship
- ❌ Napari compatibility (incomplete metadata)
- ❌ ImageJ/Fiji compatibility
- ❌ QuPath compatibility
- ❌ Python zarr library roundtrip
- ❌ Tool-specific extensions
- ❌ Migration from other formats

---

## 13. Advanced Features

### Not Supported
- ❌ **Dimension names** - No explicit dimension naming
- ❌ **Custom axes types** - Only space/channel/time from spec
- ❌ Extension points
- ❌ Must_understand flags
- ❌ Version migration
- ❌ Schema validation
- ❌ Conformance testing
- ❌ Feature discovery
- ❌ Optional feature flags
- ❌ Backward compatibility layers

---

## 14. Naming and Conventions

### Supported
- ✅ Basic camelCase for keys (multiscales, coordinateTransformations)

### Not Supported
- ❌ **Full camelCase compliance** - Some legacy naming may exist
- ❌ Case-insensitive filesystem handling
- ❌ Name collision detection
- ❌ Alphanumeric-only validation
- ❌ Path separator validation
- ❌ Reserved name checking

---

## 15. Specification Compliance

### Supported
- ✅ Zarr v3 format (MUST requirement met)
- ✅ Basic group structure
- ✅ Minimal multiscales metadata

### Not Supported (MUST requirements)
- ❌ **`dimension_names` in array zarr.json** - REQUIRED by spec but missing
- ❌ **Multiple resolutions** - SHOULD have pyramid
- ❌ **Scale transformations with actual values** - Currently all 1.0
- ❌ Units for all axes - Only spatial have units
- ❌ Proper coordinateTransformations ordering
- ❌ Metadata consistency across hierarchy

### Not Supported (SHOULD requirements)
- ❌ Named multiscales
- ❌ Type field in multiscales
- ❌ Metadata field in multiscales
- ❌ Unit specifications for all axes
- ❌ Physical pixel sizes
- ❌ Downsampling documentation
- ❌ Version tracking

---

## 16. Metadata Versioning

### Supported
- ✅ Version "0.5" in ome metadata

### Not Supported
- ❌ Version migration scripts
- ❌ Multiple version support
- ❌ Version detection
- ❌ Forward/backward compatibility handling
- ❌ Version-specific feature flags
- ❌ Deprecated feature handling
- ❌ Transitional metadata versioning

---

## 17. Error Handling and Validation

### Not Supported
- ❌ Metadata validation against spec
- ❌ Schema validation (JSON schema)
- ❌ Conformance testing
- ❌ Error reporting for invalid metadata
- ❌ Warning for incomplete metadata
- ❌ Strict mode vs permissive mode
- ❌ Validation tools integration
- ❌ Automated compliance checking

---

## Summary Statistics

**OME-NGFF Features:**
- **Multiscales**: 10% (single resolution, minimal metadata)
- **Axes**: 20% (only c, y, x; no t, z)
- **Transformations**: 10% (scale only, hardcoded)
- **Labels**: 0% (not implemented)
- **Plates**: 0% (not implemented)
- **Wells**: 0% (not implemented)
- **OMERO**: 0% (not implemented)
- **Physical units**: 5% (micrometer only)
- **Multi-resolution**: 0% (single resolution only)

**Overall Compliance: ~5-10%** of full OME-NGFF v0.5 specification

---

## Critical Missing Features

### For Basic OME-NGFF Compliance:
1. ❌ **Multi-resolution pyramids** - Core feature
2. ❌ **`dimension_names` in array metadata** - REQUIRED by spec
3. ❌ **Actual physical pixel sizes** - Currently all 1.0
4. ❌ **Z-dimension support** - For 3D microscopy
5. ❌ **Time dimension support** - For time-lapse imaging

### For Intermediate Compliance:
6. ❌ **Labels/segmentation** - Essential for analysis
7. ❌ **Reading OME-Zarr** - Roundtrip support
8. ❌ **OMERO rendering metadata** - Visualization
9. ❌ **Custom units and transformations**
10. ❌ **Multiple channel types**

### For Full Compliance:
11. ❌ **High-content screening (plates/wells)**
12. ❌ **bioformats2raw layout**
13. ❌ **All axis types and units**
14. ❌ **Complete metadata ecosystem**

---

## Implementation Notes

1. **Single Resolution Only**: Most critical limitation - pyramids are core to OME-NGFF
2. **Write-Only**: Cannot read back or validate OME-Zarr files
3. **Minimal Dimensions**: Only 2D/3D images, no time or Z-stacks
4. **Hardcoded Values**: Units, scales, transformations all use defaults
5. **No Visualization**: Missing all OMERO rendering metadata
6. **No Segmentation**: Labels feature completely absent
7. **No HCS**: No plate/well support for screening datasets
8. **Missing Requirement**: `dimension_names` MUST be present per spec

---

## Comparison with Zarr v3 Gap

**OME-NGFF builds on Zarr v3** and adds biology-specific metadata:
- **Zarr v3 implementation**: ~15-20% (see ZARR_V3_UNSUPPORTED_FEATURES.md)
- **OME-NGFF additions**: ~5-10% of biology metadata
- **Combined**: A basic single-resolution writer with minimal OME metadata

**Key gaps beyond Zarr v3:**
- Image pyramids (resolution levels)
- Labels and segmentation
- High-content screening
- Rendering/visualization metadata
- Physical coordinates and units
- Time-series and volumetric data

---

## Future Work Priorities

Based on OME-NGFF spec and typical usage:

### High Priority:
1. **Multi-resolution pyramids** - Core OME-NGFF feature
2. **`dimension_names` in arrays** - Required by spec
3. **Z-dimension** - For 3D microscopy
4. **Time dimension** - For time-lapse
5. **Actual pixel sizes** - Physical measurements

### Medium Priority:
6. **Labels/segmentation** - Analysis workflows
7. **Reading OME-Zarr** - Roundtrip support
8. **OMERO metadata** - Visualization
9. **Custom units** - Scientific accuracy
10. **More data types** - int8/16/32/64 for labels

### Lower Priority:
11. **HCS plates/wells** - Specialized use case
12. **bioformats2raw** - Migration tool
13. **Advanced transformations** - Complex coordinate systems
14. **Extensions** - Future-proofing

---

**Document Version:** 1.0  
**Specification Reference:** https://ngff.openmicroscopy.org/0.5/  
**Related Document:** ZARR_V3_UNSUPPORTED_FEATURES.md
