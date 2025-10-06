/* save to zarr
 *
 * 5/10/25
 * 	- initial implementation using zarrs library
 */

/*

	This file is part of VIPS.

	VIPS is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2 of the Lic * Chunking + sharding: Chunks grouped in shards (e.g., 128×128×3 chunks 
 *   in 256×256×3 shards → 4 chunks per shard file)
 *
 * Supported data types are: uint8, uint16, uint32, uint64*, int8, int16, 
 * int32, int64*, float32, float64, complex64, complex128.
 * (*uint64 and int64 supported via FFI but not as native VIPS formats)
 *
 * If @ome_zarr is TRUE, the output will conform to the OME-Zarror
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
	02110-1301  USA

 */

/*

	These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>
#include <vips/internal.h>

#include "pforeign.h"
#include "../rust/zarrs_wrapper.h"

typedef struct _VipsForeignSaveZarr {
	VipsForeignSave parent_object;

	char *filename;
	gboolean ome_zarr;
	gboolean pyramid;
	int chunk_height;
	int chunk_width;
	int chunk_bands;
	int shard_height;
	int shard_width;
	int shard_bands;
	VipsForeignZarrCompression compression;
	int gzip_level;
	int zstd_level;
	int blosc_clevel;
	VipsForeignZarrBloscShuffle blosc_shuffle;
	int blosc_typesize;
	int blosc_blocksize;
	
	/* Streaming write context */
	VipsZarrHandle zarr_handle;
} VipsForeignSaveZarr;

typedef VipsForeignSaveClass VipsForeignSaveZarrClass;

G_DEFINE_TYPE(VipsForeignSaveZarr, vips_foreign_save_zarr,
	VIPS_TYPE_FOREIGN_SAVE);

/* Map VipsImage format to zarr data type code
 * 0=uint8, 1=uint16, 2=uint32, 3=float32, 4=float64
 * 5=int8, 6=int16, 7=int32, 8=uint64, 9=int64
 * 10=complex64, 11=complex128
 */
static int
vips_format_to_zarr_type(VipsBandFormat format)
{
	switch (format) {
	case VIPS_FORMAT_UCHAR:
		return 0;
	case VIPS_FORMAT_USHORT:
		return 1;
	case VIPS_FORMAT_UINT:
		return 2;
	case VIPS_FORMAT_FLOAT:
		return 3;
	case VIPS_FORMAT_DOUBLE:
		return 4;
	case VIPS_FORMAT_CHAR:
		return 5;
	case VIPS_FORMAT_SHORT:
		return 6;
	case VIPS_FORMAT_INT:
		return 7;
	/* Note: VIPS doesn't have native uint64, so we can't map it directly */
	/* Note: VIPS doesn't have native int64, so we can't map it directly */
	case VIPS_FORMAT_COMPLEX:
		return 10;
	case VIPS_FORMAT_DPCOMPLEX:
		return 11;
	default:
		return -1;
	}
}

/* Called for each strip/region of the image during saving.
 * Write the region data to the zarr array via the streaming API.
 */
static int
vips_foreign_save_zarr_block(VipsRegion *region, VipsRect *area, void *a)
{
	VipsForeignSaveZarr *zarr = (VipsForeignSaveZarr *) a;
	
	/* Get pointer to the start of this region's data */
	VipsPel *p = VIPS_REGION_ADDR(region, area->left, area->top);
	
	/* Calculate the size of this region in bytes */
	size_t line_size = area->width * VIPS_IMAGE_SIZEOF_PEL(region->im);
	size_t region_size = line_size * area->height;
	
	/* For non-contiguous regions, we need to copy to a contiguous buffer */
	VipsPel *data;
	gboolean need_free = FALSE;
	
	if (VIPS_REGION_LSKIP(region) == line_size) {
		/* Data is contiguous, use directly */
		data = p;
	}
	else {
		/* Data is not contiguous, need to copy */
		data = g_malloc(region_size);
		need_free = TRUE;
		
		VipsPel *q = data;
		for (int y = 0; y < area->height; y++) {
			VipsPel *row = VIPS_REGION_ADDR(region, area->left, area->top + y);
			memcpy(q, row, line_size);
			q += line_size;
		}
	}
	
	/* Write the region to zarr */
	int result = vips_zarr_write_region(
		zarr->zarr_handle,
		area->left,
		area->top,
		area->width,
		area->height,
		data,
		region_size
	);
	
	if (need_free)
		g_free(data);
	
	return result;
}

static int
vips_foreign_save_zarr_build(VipsObject *object)
{
	VipsForeignSave *save = (VipsForeignSave *) object;
	VipsForeignSaveZarr *zarr = (VipsForeignSaveZarr *) object;
	VipsImage *in;
	
	int data_type;
	VipsImage *layer;
	int level;
	int num_levels;

	/* Call parent build first to initialize save->ready
	 */
	if (VIPS_OBJECT_CLASS(vips_foreign_save_zarr_parent_class)->build(object))
		return -1;

	/* Now we can safely access save->ready
	 */
	in = save->ready;

	/* Check if zarrs is available.
	 */
	if (!vips_zarr_available()) {
		vips_error("zarrsave", "%s", "zarrs library not available");
		return -1;
	}

	/* Get the data type code.
	 */
	data_type = vips_format_to_zarr_type(in->BandFmt);
	if (data_type < 0) {
		vips_error("zarrsave", 
			"unsupported band format %s",
			vips_enum_nick(VIPS_TYPE_BAND_FORMAT, in->BandFmt));
		return -1;
	}

	/* Pyramid implies ome_zarr */
	if (zarr->pyramid)
		zarr->ome_zarr = TRUE;

	/* Build pyramid levels if requested.
	 */
	layer = in;
	g_object_ref(layer);
	num_levels = 0;
	
	for (level = 0;; level++) {
		char level_path[256];
		
		/* For OME-Zarr (pyramid or not), use numbered paths (0, 1, 2, ...).
		 * For non-OME-Zarr, store directly at root.
		 */
		if (zarr->ome_zarr) {
			snprintf(level_path, sizeof(level_path), "%s/%d", 
				zarr->filename, level);
		} else {
			/* Non-OME: single level stored at root */
			snprintf(level_path, sizeof(level_path), "%s", 
				zarr->filename);
		}
		
		/* Initialize the zarr array for this level.
		 * For pyramids, we don't want OME metadata on individual levels,
		 * only on the root group at the end.
		 */
		zarr->zarr_handle = vips_zarr_init_array(
			level_path,
			layer->Xsize,
			layer->Ysize,
			layer->Bands,
			data_type,
			FALSE,  /* Don't write OME metadata per level */
			zarr->chunk_height,
			zarr->chunk_width,
			zarr->chunk_bands,
			zarr->shard_height,
			zarr->shard_width,
			zarr->shard_bands,
			zarr->compression,
			zarr->gzip_level,
			zarr->zstd_level,
			zarr->blosc_clevel,
			zarr->blosc_shuffle,
			zarr->blosc_typesize,
			zarr->blosc_blocksize
		);
		
		if (!zarr->zarr_handle) {
			vips_error("zarrsave", "%s", "failed to initialize zarr array");
			g_object_unref(layer);
			return -1;
		}

		/* Stream the image data.
		 */
		if (vips_sink_disc(layer, vips_foreign_save_zarr_block, zarr)) {
			vips_zarr_finalize_no_metadata(zarr->zarr_handle);
			zarr->zarr_handle = NULL;
			g_object_unref(layer);
			return -1;
		}

		/* Finalize the zarr array without OME metadata (will write it once at the end).
		 */
		if (vips_zarr_finalize_no_metadata(zarr->zarr_handle) < 0) {
			vips_error("zarrsave", "%s", "failed to finalize zarr array");
			zarr->zarr_handle = NULL;
			g_object_unref(layer);
			return -1;
		}
		
		zarr->zarr_handle = NULL;
		num_levels++;

		/* If not pyramid, we're done after first level.
		 */
		if (!zarr->pyramid)
			break;

		/* Stop if image is small enough (smaller than a single chunk).
		 */
		if (layer->Xsize < 256 && layer->Ysize < 256)
			break;

		/* Create the next pyramid level by downsampling by 2.
		 */
		VipsImage *shrunk;
		if (vips_shrink(layer, &shrunk, 2, 2, NULL)) {
			g_object_unref(layer);
			return -1;
		}
		
		g_object_unref(layer);
		layer = shrunk;
	}
	
	g_object_unref(layer);

	/* Write OME-Zarr metadata for pyramids, or single-level OME metadata.
	 */
	if (zarr->ome_zarr) {
		if (zarr->pyramid) {
			/* Write pyramid metadata with all levels */
			if (vips_zarr_write_pyramid_metadata(zarr->filename, 
					num_levels, in->Xsize, in->Ysize, in->Bands, data_type) < 0) {
				vips_error("zarrsave", "%s", "failed to write pyramid metadata");
				return -1;
			}
		} else {
			/* For non-pyramid OME-Zarr, we need to write single-level metadata.
			 * The metadata was not written during finalize, so we need to do it now.
			 * We'll use the pyramid function with num_levels=1.
			 */
			if (vips_zarr_write_pyramid_metadata(zarr->filename, 
					1, in->Xsize, in->Ysize, in->Bands, data_type) < 0) {
				vips_error("zarrsave", "%s", "failed to write OME metadata");
				return -1;
			}
		}
	}

	return 0;
}

static const char *vips_foreign_save_zarr_suffs[] = {
	".zarr",
	NULL
};

static void
vips_foreign_save_zarr_dispose(GObject *gobject)
{
	VipsForeignSaveZarr *zarr = (VipsForeignSaveZarr *) gobject;

	/* Clean up zarr handle if still open */
	if (zarr->zarr_handle) {
		vips_zarr_finalize(zarr->zarr_handle);
		zarr->zarr_handle = NULL;
	}

	G_OBJECT_CLASS(vips_foreign_save_zarr_parent_class)->dispose(gobject);
}

static void
vips_foreign_save_zarr_class_init(VipsForeignSaveZarrClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignSaveClass *save_class = (VipsForeignSaveClass *) class;

	gobject_class->dispose = vips_foreign_save_zarr_dispose;
	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "zarrsave";
	object_class->description = _("save image to zarr format");
	object_class->build = vips_foreign_save_zarr_build;

	foreign_class->suffs = vips_foreign_save_zarr_suffs;

	save_class->saveable = VIPS_FOREIGN_SAVEABLE_ANY;

	VIPS_ARG_STRING(class, "filename", 1,
		_("Filename"),
		_("Filename to save to"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, filename),
		NULL);

	VIPS_ARG_BOOL(class, "ome_zarr", 20,
		_("OME-Zarr"),
		_("Write OME-Zarr (Open Microscopy Environment) compatible metadata"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, ome_zarr),
		FALSE);

	VIPS_ARG_BOOL(class, "pyramid", 19,
		_("Pyramid"),
		_("Write an image pyramid (OME-Zarr multiscales)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, pyramid),
		FALSE);

	VIPS_ARG_INT(class, "chunk_height", 21,
		_("Chunk height"),
		_("Chunk height (0 for full image height)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, chunk_height),
		0, 100000000, 0);

	VIPS_ARG_INT(class, "chunk_width", 22,
		_("Chunk width"),
		_("Chunk width (0 for full image width)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, chunk_width),
		0, 100000000, 0);

	VIPS_ARG_INT(class, "chunk_bands", 23,
		_("Chunk bands"),
		_("Chunk bands (0 for all bands)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, chunk_bands),
		0, 100000, 0);

	VIPS_ARG_INT(class, "shard_height", 24,
		_("Shard height"),
		_("Shard height (0 for no sharding)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, shard_height),
		0, 100000000, 0);

	VIPS_ARG_INT(class, "shard_width", 25,
		_("Shard width"),
		_("Shard width (0 for no sharding)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, shard_width),
		0, 100000000, 0);

	VIPS_ARG_INT(class, "shard_bands", 26,
		_("Shard bands"),
		_("Shard bands (0 for no sharding)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, shard_bands),
		0, 100000, 0);

	VIPS_ARG_ENUM(class, "compression", 27,
		_("Compression"),
		_("Compression codec (gzip, zstd, or blosc variants)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, compression),
		VIPS_TYPE_FOREIGN_ZARR_COMPRESSION,
		VIPS_FOREIGN_ZARR_COMPRESSION_GZIP);

	VIPS_ARG_INT(class, "gzip_level", 28,
		_("Gzip level"),
		_("Gzip compression level (1-9, 0 for default)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, gzip_level),
		0, 9, 0);

	VIPS_ARG_INT(class, "zstd_level", 29,
		_("Zstd level"),
		_("Zstd compression level (1-22, 0 for default)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, zstd_level),
		0, 22, 0);

	VIPS_ARG_INT(class, "blosc_clevel", 30,
		_("Blosc compression level"),
		_("Blosc compression level (0-9, 0 for default)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, blosc_clevel),
		0, 9, 0);

	VIPS_ARG_ENUM(class, "blosc_shuffle", 31,
		_("Blosc shuffle"),
		_("Blosc shuffle mode"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, blosc_shuffle),
		VIPS_TYPE_FOREIGN_ZARR_BLOSC_SHUFFLE,
		VIPS_FOREIGN_ZARR_BLOSC_SHUFFLE);

	VIPS_ARG_INT(class, "blosc_typesize", 32,
		_("Blosc typesize"),
		_("Blosc typesize (0 for automatic based on data type)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, blosc_typesize),
		0, 32, 0);

	VIPS_ARG_INT(class, "blosc_blocksize", 33,
		_("Blosc blocksize"),
		_("Blosc blocksize in bytes (0 for automatic)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, blosc_blocksize),
		0, INT_MAX, 0);
}

static void
vips_foreign_save_zarr_init(VipsForeignSaveZarr *zarr)
{
	zarr->zarr_handle = NULL;
}

/**
 * vips_zarrsave: (method)
 * @in: image to save
 * @filename: file to write to
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * * @ome_zarr: %gboolean, write OME-Zarr compatible metadata
 * * @chunk_height: %gint, chunk height (0 for full image height)
 * * @chunk_width: %gint, chunk width (0 for full image width)
 * * @chunk_bands: %gint, chunk bands (0 for all bands)
 * * @shard_height: %gint, shard height (0 for no sharding)
 * * @shard_width: %gint, shard width (0 for no sharding)
 * * @shard_bands: %gint, shard bands (0 for no sharding)
 * * @compression: #VipsForeignZarrCompression, compression codec
 * * @gzip_level: %gint, gzip compression level (1-9, 0 for default of 5)
 * * @zstd_level: %gint, zstd compression level (1-22, 0 for default of 3)
 *
 * Write @in to a Zarr v3 format array at @filename.
 *
 * The Zarr format is a specification for chunked, compressed, 
 * N-dimensional arrays. This operation creates a Zarr v3 compatible
 * array store on the filesystem.
 *
 * The image is written with shape [height, width, bands]. By default,
 * it is saved as a single chunk with gzip compression.
 *
 * Use @compression to select the compression codec:
 * - GZIP (default): Good compression with wide compatibility
 * - ZSTD: Better compression ratios and faster decompression
 *
 * Use @gzip_level to control gzip compression (1-9, default 5):
 * - Lower values (1-3): Faster compression, larger files
 * - Medium values (4-6): Balanced speed and compression
 * - Higher values (7-9): Better compression, slower speed
 *
 * Use @zstd_level to control zstd compression (1-22, default 3):
 * - Lower values (1-3): Very fast compression
 * - Medium values (4-9): Good balance of speed and compression
 * - Higher values (10-22): Maximum compression, slower speed
 *
 * Chunking divides the array into regular blocks for efficient access:
 * - If @chunk_height, @chunk_width, and @chunk_bands are specified (all > 0),
 *   the array will be divided into chunks of that size
 * - If chunk parameters are 0 (default), the entire image is one chunk
 * - Chunks are the basic unit of I/O in Zarr
 *
 * Sharding is an optimization that groups multiple chunks together:
 * - If @shard_height, @shard_width, and @shard_bands are specified (all > 0),
 *   multiple chunks will be grouped into larger shard files
 * - Sharding reduces file count and improves cloud storage performance
 * - Shard dimensions should be multiples of chunk dimensions
 * - If sharding is enabled, chunks must also be specified
 *
 * Example configurations:
 * - No chunking (default): Full image as one file
 * - Chunking only: Multiple chunk files (e.g., 256×256×3 chunks)
 * - Chunking + sharding: Chunks grouped in shards (e.g., 128×128×3 chunks 
 *   in 256×256×3 shards → 4 chunks per shard file)
 *
 * Supported data types are: uint8, uint16, uint32, float32, float64.
 *
 * If @ome_zarr is TRUE, the output will conform to the OME-Zarr
 * (Open Microscopy Environment - Next Generation File Format) 
 * specification v0.5, with proper multiscales metadata, axes 
 * definitions, and coordinate transformations. The array will be
 * stored in a '0' subdirectory with group-level metadata.
 *
 * See also: vips_image_write_to_file().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_zarrsave(VipsImage *in, const char *filename, ...)
{
	va_list ap;
	int result;

	va_start(ap, filename);
	result = vips_call_split("zarrsave", ap, in, filename);
	va_end(ap);

	return result;
}

/* VipsForeignSaveZarrFile class for registration */
typedef VipsForeignSaveZarr VipsForeignSaveZarrFile;
typedef VipsForeignSaveZarrClass VipsForeignSaveZarrFileClass;

G_DEFINE_TYPE(VipsForeignSaveZarrFile, vips_foreign_save_zarr_file,
	vips_foreign_save_zarr_get_type());

static void
vips_foreign_save_zarr_file_class_init(VipsForeignSaveZarrFileClass *class)
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	object_class->nickname = "zarrsave_file";
	object_class->description = _("save image to zarr file");
}

static void
vips_foreign_save_zarr_file_init(VipsForeignSaveZarrFile *file)
{
}
