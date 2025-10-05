/* save to zarr
 *
 * 5/10/25
 * 	- initial implementation using zarrs library
 */

/*

	This file is part of VIPS.

	VIPS is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
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
	int chunk_height;
	int chunk_width;
	int chunk_bands;
	int shard_height;
	int shard_width;
	int shard_bands;
	VipsForeignZarrCompression compression;
} VipsForeignSaveZarr;

typedef VipsForeignSaveClass VipsForeignSaveZarrClass;

G_DEFINE_TYPE(VipsForeignSaveZarr, vips_foreign_save_zarr,
	VIPS_TYPE_FOREIGN_SAVE);

/* Map VipsImage format to zarr data type code
 * 0=uint8, 1=uint16, 2=uint32, 3=float32, 4=float64
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
	default:
		return -1;
	}
}

static int
vips_foreign_save_zarr_build(VipsObject *object)
{
	VipsForeignSave *save = (VipsForeignSave *) object;
	VipsForeignSaveZarr *zarr = (VipsForeignSaveZarr *) object;
	VipsImage *in;
	
	int data_type;
	size_t data_len;
	void *data;

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

	/* Get the image data.
	 * We need the whole image in memory.
	 */
	if (vips_image_wio_input(in))
		return -1;

	data = VIPS_IMAGE_ADDR(in, 0, 0);
	data_len = VIPS_IMAGE_SIZEOF_IMAGE(in);

	/* Write the zarr array.
	 */
	if (vips_zarr_write_array(
			zarr->filename,
			in->Xsize,
			in->Ysize,
			in->Bands,
			data_type,
			data,
			data_len,
			zarr->ome_zarr,
			zarr->chunk_height,
			zarr->chunk_width,
			zarr->chunk_bands,
			zarr->shard_height,
			zarr->shard_width,
			zarr->shard_bands,
			zarr->compression) < 0) {
		vips_error("zarrsave", "%s", "failed to write zarr array");
		return -1;
	}

	return 0;
}

static const char *vips_foreign_save_zarr_suffs[] = {
	".zarr",
	NULL
};

static void
vips_foreign_save_zarr_class_init(VipsForeignSaveZarrClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignSaveClass *save_class = (VipsForeignSaveClass *) class;

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
		_("Compression codec (gzip or zstd)"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveZarr, compression),
		VIPS_TYPE_FOREIGN_ZARR_COMPRESSION,
		VIPS_FOREIGN_ZARR_COMPRESSION_GZIP);
}

static void
vips_foreign_save_zarr_init(VipsForeignSaveZarr *zarr)
{
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
