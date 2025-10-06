/* load zarr from a file
 *
 * 6/10/25
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

typedef struct _VipsForeignLoadZarr {
	VipsForeignLoad parent_object;

	/* Set by subclasses.
	 */
	char *filename;
	
	int level;
	VipsZarrReadHandle zarr_handle;

} VipsForeignLoadZarr;

typedef VipsForeignLoadClass VipsForeignLoadZarrClass;

G_DEFINE_TYPE(VipsForeignLoadZarr, vips_foreign_load_zarr,
	VIPS_TYPE_FOREIGN_LOAD);

/* Convert zarr data type code to VIPS band format
 */
static VipsBandFormat
vips_zarr_type_to_format(int data_type)
{
	switch (data_type) {
		case 0: return VIPS_FORMAT_UCHAR;
		case 1: return VIPS_FORMAT_USHORT;
		case 2: return VIPS_FORMAT_UINT;
		case 3: return VIPS_FORMAT_FLOAT;
		case 4: return VIPS_FORMAT_DOUBLE;
		case 5: return VIPS_FORMAT_CHAR;
		case 6: return VIPS_FORMAT_SHORT;
		case 7: return VIPS_FORMAT_INT;
		case 10: return VIPS_FORMAT_COMPLEX;
		case 11: return VIPS_FORMAT_DPCOMPLEX;
		default: return VIPS_FORMAT_NOTSET;
	}
}

static void
vips_foreign_load_zarr_dispose(GObject *gobject)
{
	VipsForeignLoadZarr *zarr = (VipsForeignLoadZarr *) gobject;

	if (zarr->zarr_handle) {
		vips_zarr_close(zarr->zarr_handle);
		zarr->zarr_handle = NULL;
	}

	G_OBJECT_CLASS(vips_foreign_load_zarr_parent_class)->dispose(gobject);
}

static int
vips_foreign_load_zarr_is_a(const char *filename)
{
	/* Check if it's a directory ending in .zarr or .zarr/
	 */
	return g_file_test(filename, G_FILE_TEST_IS_DIR) &&
		(g_str_has_suffix(filename, ".zarr") ||
		 g_str_has_suffix(filename, ".zarr/"));
}

static VipsForeignFlags
vips_foreign_load_zarr_get_flags_filename(const char *filename)
{
	/* Zarr supports partial reads.
	 */
	return VIPS_FOREIGN_PARTIAL;
}

static VipsForeignFlags
vips_foreign_load_zarr_get_flags(VipsForeignLoad *load)
{
	return VIPS_FOREIGN_PARTIAL;
}

static int
vips_foreign_load_zarr_header(VipsForeignLoad *load)
{
	VipsForeignLoadZarr *zarr = (VipsForeignLoadZarr *) load;
	VipsImage *out = load->out;
	
	uint64_t width, height, bands;
	int data_type;
	VipsBandFormat format;
	char level_path[256];

	/* Check if zarrs is available.
	 */
	if (!vips_zarr_available()) {
		vips_error("zarrload", "%s", "zarrs library not available");
		return -1;
	}

	/* Build path to the requested level
	 */
	if (zarr->level > 0) {
		snprintf(level_path, sizeof(level_path), "%s/%d", 
			zarr->filename, zarr->level);
	} else {
		/* Try level 0 first, fall back to root */
		snprintf(level_path, sizeof(level_path), "%s/0", zarr->filename);
		
		/* Check if level 0 exists */
		if (!g_file_test(level_path, G_FILE_TEST_IS_DIR)) {
			/* Fall back to root */
			snprintf(level_path, sizeof(level_path), "%s", zarr->filename);
		}
	}

	/* Open the zarr array
	 */
	zarr->zarr_handle = vips_zarr_open(level_path);
	if (!zarr->zarr_handle) {
		vips_error("zarrload", "failed to open zarr array at path: %s", level_path);
		return -1;
	}

	/* Get metadata
	 */
	if (vips_zarr_get_metadata(zarr->zarr_handle, 
			&width, &height, &bands, &data_type) < 0) {
		vips_error("zarrload", "%s", "failed to get zarr metadata");
		return -1;
	}

	/* Convert data type
	 */
	format = vips_zarr_type_to_format(data_type);
	if (format == VIPS_FORMAT_NOTSET) {
		vips_error("zarrload", "unsupported data type %d", data_type);
		return -1;
	}

	/* Set image properties
	 */
	vips_image_init_fields(out,
		width, height, bands,
		format,
		VIPS_CODING_NONE,
		VIPS_INTERPRETATION_MULTIBAND,
		1.0, 1.0);

	/* Set demand hint for random access (like openslideload)
	 */
	if (vips_image_pipelinev(out, VIPS_DEMAND_STYLE_SMALLTILE, NULL))
		return -1;

	return 0;
}

/* Generate a region of pixels
 */
static int
vips_foreign_load_zarr_generate(VipsRegion *out_region,
	void *seq, void *a, void *b, gboolean *stop)
{
	VipsForeignLoadZarr *zarr = (VipsForeignLoadZarr *) a;
	VipsRect *r = &out_region->valid;
	VipsImage *im = out_region->im;
	
	size_t region_size;
	
	/* Calculate region size in bytes
	 */
	region_size = (size_t) r->width * r->height * im->Bands * 
		VIPS_IMAGE_SIZEOF_ELEMENT(im);

	/* Read the region from zarr
	 */
	if (vips_zarr_read_region(zarr->zarr_handle,
			r->left, r->top, r->width, r->height,
			VIPS_REGION_ADDR(out_region, r->left, r->top),
			region_size) < 0) {
		vips_error("zarrload", "%s", "failed to read region");
		return -1;
	}

	return 0;
}

static int
vips_foreign_load_zarr_load(VipsForeignLoad *load)
{
	VipsForeignLoadZarr *zarr = (VipsForeignLoadZarr *) load;
	VipsImage **t = (VipsImage **) vips_object_local_array(VIPS_OBJECT(load), 3);

	/* Read to t[0], adding a cache.
	 */
	t[0] = vips_image_new();
	
	/* Copy header properties from load->out to t[0]
	 */
	if (vips_image_pipelinev(t[0], VIPS_DEMAND_STYLE_SMALLTILE, NULL))
		return -1;
	
	vips_image_init_fields(t[0],
		load->out->Xsize, load->out->Ysize, load->out->Bands,
		load->out->BandFmt,
		load->out->Coding,
		load->out->Type,
		load->out->Xres, load->out->Yres);
	
	if (vips_image_generate(t[0],
			NULL, vips_foreign_load_zarr_generate, NULL,
			zarr, NULL))
		return -1;

	/* Copy to output with a tile cache. Use 128x128 tiles.
	 */
	if (vips_tilecache(t[0], &t[1],
			"tile_width", 128,
			"tile_height", 128,
			"max_tiles", -1,
			"threaded", TRUE,
			NULL))
		return -1;

	if (vips_image_write(t[1], load->real))
		return -1;

	return 0;
}

/* Suffix list for zarr files
 */
static const char *vips_foreign_load_zarr_suffs[] = {
	".zarr",
	NULL
};

static void
vips_foreign_load_zarr_class_init(VipsForeignLoadZarrClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->dispose = vips_foreign_load_zarr_dispose;
	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "zarrload_base";
	object_class->description = _("load zarr");

	/* We're the only loader for .zarr directories, so high priority
	 */
	foreign_class->priority = 200;

	/* Base class intentionally has no is_a or suffs set.
	 * This prevents it from matching files during loader discovery.
	 * The warning "loader zarrload_base has no is_a method and no suffix list"
	 * is expected and harmless - only the file subclass should match files.
	 */

	load_class->get_flags_filename = vips_foreign_load_zarr_get_flags_filename;
	load_class->get_flags = vips_foreign_load_zarr_get_flags;
	load_class->header = vips_foreign_load_zarr_header;
	load_class->load = vips_foreign_load_zarr_load;

	VIPS_ARG_INT(class, "level", 20,
		_("Level"),
		_("Pyramid level to load"),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET(VipsForeignLoadZarr, level),
		0, 100, 0);
}

static void
vips_foreign_load_zarr_init(VipsForeignLoadZarr *zarr)
{
	zarr->level = 0;
	zarr->zarr_handle = NULL;
}

typedef struct _VipsForeignLoadZarrFile {
	VipsForeignLoadZarr parent_object;

	char *filename;

} VipsForeignLoadZarrFile;

typedef VipsForeignLoadZarrClass VipsForeignLoadZarrFileClass;

G_DEFINE_TYPE(VipsForeignLoadZarrFile, vips_foreign_load_zarr_file,
	vips_foreign_load_zarr_get_type());

static int
vips_foreign_load_zarr_file_build(VipsObject *object)
{
	VipsForeignLoadZarr *zarr = (VipsForeignLoadZarr *) object;
	VipsForeignLoadZarrFile *file = (VipsForeignLoadZarrFile *) object;
	
	if (file->filename)
		zarr->filename = g_strdup(file->filename);

	/* Call parent build - this will trigger header() which needs zarr->filename
	 */
	if (VIPS_OBJECT_CLASS(vips_foreign_load_zarr_file_parent_class)->build(object))
		return -1;

	return 0;
}

static void
vips_foreign_load_zarr_file_class_init(VipsForeignLoadZarrFileClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "zarrload";
	object_class->build = vips_foreign_load_zarr_file_build;
	object_class->description = _("load zarr from file");

	foreign_class->suffs = vips_foreign_load_zarr_suffs;

	load_class->is_a = vips_foreign_load_zarr_is_a;

	VIPS_ARG_STRING(class, "filename", 1,
		_("Filename"),
		_("Filename to load from"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignLoadZarrFile, filename),
		NULL);
}

static void
vips_foreign_load_zarr_file_init(VipsForeignLoadZarrFile *file)
{
}
