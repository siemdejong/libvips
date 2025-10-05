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
			data_len) < 0) {
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
 * Write @in to a Zarr v3 format array at @filename.
 *
 * The Zarr format is a specification for chunked, compressed, 
 * N-dimensional arrays. This operation creates a Zarr v3 compatible
 * array store on the filesystem.
 *
 * The image is written with shape [height, width, bands] and saved
 * as a single chunk with gzip compression.
 *
 * Supported data types are: uint8, uint16, uint32, float32, float64.
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
