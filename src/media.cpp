/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * media.cpp: 
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "runtime.h"
#include "media.h"
#include "error.h"
#include "downloader.h"
#include "geometry.h"
#include "clock.h"
#include "debug.h"

// still too ugly to be exposed in the header files ;-)
void image_brush_compute_pattern_matrix (cairo_matrix_t *matrix, double width, double height, int sw, int sh, 
					 Stretch stretch, AlignmentX align_x, AlignmentY align_y, Transform *transform,
					 Transform *relative_transform);


/*
 * MediaBase
 */

MediaBase::MediaBase ()
{
	source.downloader = NULL;
	source.part_name = NULL;
	source.queued = false;
	downloader = NULL;
	part_name = NULL;
	updating_size_from_media = false;
	allow_downloads = false;
	use_media_height = true;
	use_media_width = true;
	source_changed = false;
}

MediaBase::~MediaBase ()
{
	DownloaderAbort ();
}

void
MediaBase::downloader_complete (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	MediaBase *media = (MediaBase *) closure;
	
	media->DownloaderComplete ();
}

void
MediaBase::downloader_failed (EventObject *sender, EventArgs *calldata, gpointer closure)
{
	MediaBase *media = (MediaBase *) closure;
	
	media->DownloaderFailed (calldata);
}

void
MediaBase::DownloaderComplete ()
{
	// Nothing for MediaBase to do...
}

void
MediaBase::DownloaderFailed (EventArgs *args)
{
	// Nothing for MediaBase to do...
}

void
MediaBase::DownloaderAbort ()
{
	if (downloader) {
		downloader->RemoveHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
		downloader->RemoveHandler (Downloader::CompletedEvent, downloader_complete, this);
		downloader->SetWriteFunc (NULL, NULL, NULL);
		downloader->Abort ();
		downloader->unref ();
		g_free (part_name);
		downloader = NULL;
		part_name = NULL;
	}
}

void
MediaBase::SetAllowDownloads (bool allow)
{
	Surface *surface = GetSurface ();
	const char *uri;
	Downloader *dl;
	
	if ((allow_downloads && allow) || (!allow_downloads && !allow))
		return;
	
	if (allow && surface && source_changed) {
		source_changed = false;
		
		if ((uri = GetSource ()) && *uri) {
			if (!(dl = surface->CreateDownloader ())) {
				// we're shutting down
				return;
			}
			
			dl->Open ("GET", uri, GetDownloaderPolicy (uri));
			SetSource (dl, "");
			dl->unref ();
		}
	}
	
	allow_downloads = allow;
}

void
MediaBase::OnLoaded ()
{
	FrameworkElement::OnLoaded ();
	SetAllowDownloads (true);
}

void
MediaBase::SetSourceAsyncCallback ()
{
	Downloader *downloader;
	char *part_name;

	DownloaderAbort ();

	downloader = source.downloader;
	part_name = source.part_name;

	source.queued = false;
	source.downloader = NULL;
	source.part_name = NULL;
	
	if (GetSurface () == NULL)
		return;
	
	SetSourceInternal (downloader, part_name);
	
	if (downloader)
		downloader->unref ();
}

void
MediaBase::SetSourceInternal (Downloader *downloader, char *PartName)
{
	this->downloader = downloader;
	part_name = PartName;
	
	if (downloader)
		downloader->ref ();
}

void
MediaBase::set_source_async (EventObject *user_data)
{
	MediaBase *media = (MediaBase *) user_data;
	
	media->SetSourceAsyncCallback ();
}

void
MediaBase::SetSource (Downloader *downloader, const char *PartName)
{
	source_changed = false;
	
	if (source.queued) {
		if (source.downloader)
			source.downloader->unref ();

		g_free (source.part_name);
		source.downloader = NULL;
		source.part_name = NULL;
	}
	
	source.part_name = g_strdup (PartName);
	source.downloader = downloader;
	
	if (downloader)
		downloader->ref ();

	if (source.downloader && source.downloader->Completed ()) {
		SetSourceInternal (source.downloader, source.part_name);
		source.downloader->unref ();
	} else if (!source.queued) {
		AddTickCall (MediaBase::set_source_async);
		source.queued = true;
	}
}

void
MediaBase::SetSource (const char *uri)
{
	Value v(uri);
	PropertyChangedEventArgs args (MediaBase::SourceProperty, NULL, &v);
	OnPropertyChanged (&args);
}


void
MediaBase::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property == MediaBase::SourceProperty) {
		const char *uri = args->new_value ? args->new_value->AsString () : NULL;
		Surface *surface = GetSurface ();
					
		if (surface && AllowDownloads ()) {
			if (uri && *uri) {
				Downloader *dl;
				if ((dl = surface->CreateDownloader ())) {
					dl->Open ("GET", uri, GetDownloaderPolicy (uri));
					SetSource (dl, "");
					dl->unref ();
				} else {
					// we're shutting down
				}
			} else {
				SetSource (NULL, NULL);
			}
		} else {
			source_changed = true;
		}
	}
	
	if (args->property->GetOwnerType() != Type::MEDIABASE) {
		FrameworkElement::OnPropertyChanged (args);
		return;
	}
	
	NotifyListenersOfPropertyChange (args);
}

const char *
MediaBase::GetSource ()
{
	Value *value = GetValue (MediaBase::SourceProperty);
	
	return value ? value->AsString () : NULL;
}

//
// Image
//
GHashTable *Image::surface_cache = NULL;

Image::Image ()
{
	create_xlib_surface = true;
	
	pattern = NULL;
	brush = NULL;
	surface = NULL;
	loader = NULL;
	loader_err = NULL;
}

Image::~Image ()
{
	if (loader) {
		gdk_pixbuf_loader_close (GDK_PIXBUF_LOADER (loader), NULL);
		g_object_unref (loader);
	}
	CleanupSurface ();
}

void
Image::CleanupPattern ()
{
	if (pattern) {
		cairo_pattern_destroy (pattern);
		pattern = NULL;
	}
}

void
Image::CleanupSurface ()
{
	CleanupPattern ();

	if (surface) {
		surface->ref_count--;
		if (surface->ref_count == 0) {
			LOG_MEDIA ("removing %s\n", surface->filename);
			g_hash_table_remove (surface_cache, surface->filename);
			g_free (surface->filename);
			cairo_surface_destroy (surface->cairo);
			if (surface->backing_pixbuf)
				g_object_unref (surface->backing_pixbuf);
			if (surface->backing_data)
				g_free (surface->backing_data);
			g_free (surface);
		}
		
		surface = NULL;
	}
}

void
Image::UpdateProgress ()
{
	if (downloader == NULL)
		return;

	double progress = downloader->GetDownloadProgress ();
	double current = GetDownloadProgress ();
	
	SetDownloadProgress (progress);
	
	/* only emit an event if the delta is >= 0.05% */
	if (progress == 1.0 || (progress - current) > 0.0005)
		Emit (DownloadProgressChangedEvent);
}


#if SL_2_0
void
Image::SetStreamSource (ManagedStreamCallbacks *callbacks)
{
	void *buf;
	guint64 length = callbacks->Length (callbacks->handle);

	buf = (void *)g_malloc (length);

	callbacks->Read (callbacks->handle, buf, 0, length);

	PixbufWrite (buf, 0, (gint32) length);

	CleanupSurface ();
	
	if (!CreateSurface (NULL)) {
		Invalidate ();
		return;
	}
	
	updating_size_from_media = true;
	
	if (use_media_width) {
		Value *height = GetValueNoDefault (FrameworkElement::HeightProperty);

		if (!use_media_height)
			SetWidth ((double) surface->width * height->AsDouble () / (double) surface->height);
		else
			SetWidth ((double) surface->width);
	}
	
	if (use_media_height) {
		Value *width = GetValueNoDefault (FrameworkElement::WidthProperty);

		if (!use_media_width)
			SetHeight ((double) surface->height * width->AsDouble () / (double) surface->width);
		else
			SetHeight ((double) surface->height);
	}
	
	updating_size_from_media = false;
	
	if (brush) {
		// FIXME: this is wrong, we probably need to set the
		// property, or use some other mechanism, but this is
		// gross.
		PropertyChangedEventArgs args (ImageBrush::DownloadProgressProperty, NULL, 
					       brush->GetValue (ImageBrush::DownloadProgressProperty));
		
		brush->OnPropertyChanged (&args);
	} else
		Invalidate ();
}
#endif

bool 
Image::IsSurfaceCached ()
{
	const char *uri;
	bool found;

	if (!downloader)
		return false;

	if (strcmp (part_name, "") == 0)
		uri = downloader->GetUri ();
	else
		uri = downloader->GetDownloadedFilename (part_name);

	found = uri && surface_cache && g_hash_table_lookup (surface_cache, uri);

	LOG_MEDIA ("%s cache for (%s)\n", found ? "found" : "no", uri);

	return found;
}

void
Image::SetSourceInternal (Downloader *downloader, char *PartName)
{
	// We need to actually download something
	MediaBase::SetSourceInternal (downloader, PartName);
	
	// If the uri the we are requesting is already cached 
	// short circuit the request and use the cached image
	if (IsSurfaceCached ()) {
		DownloaderComplete ();
		SetDownloadProgress (1.0);
		
		Emit (DownloadProgressChangedEvent);
		
		MediaBase::SetSourceInternal (NULL, PartName);
		downloader->Abort ();
		downloader->unref ();
		return;
	} else if (!downloader) {
		CleanupSurface ();
		Invalidate ();
		return;
	}
	
	downloader->AddHandler (Downloader::CompletedEvent, downloader_complete, this);
	downloader->AddHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
	
	if (downloader->Started () || downloader->Completed ()) {
		if (downloader->Completed ())
			DownloaderComplete ();
		
		UpdateProgress ();
	} else {
		downloader->SetWriteFunc (pixbuf_write, size_notify, this);
		
		// Image::SetSource() is already async, so we don't need another
		// layer of asyncronicity... it is safe to call SendNow() here.
		downloader->SendNow ();
	}
}

void
Image::SetSource (Downloader *downloader, const char *PartName)
{
	loader = NULL;
	MediaBase::SetSource (downloader, PartName);
}

void
Image::PixbufWrite (void *buf, gint32 offset, gint32 n)
{
	UpdateProgress ();
	if (loader == NULL)
		loader = gdk_pixbuf_loader_new ();

	if (!loader_err) {
		gdk_pixbuf_loader_write (GDK_PIXBUF_LOADER (loader), (const guchar *)buf, n, &loader_err);

		if (loader_err) {
			gdk_pixbuf_loader_close (GDK_PIXBUF_LOADER (loader), NULL);
		}
	}
}

void
Image::DownloaderComplete ()
{
	char *uri;

	downloader->RemoveHandler (Downloader::DownloadFailedEvent, downloader_failed, this);
	downloader->RemoveHandler (Downloader::CompletedEvent, downloader_complete, this);

	if (strcmp (part_name, "") == 0)
		uri = g_strdup (downloader->GetUri ());
	else
		uri = g_strdup (downloader->GetDownloadedFilename (part_name));

	if (!surface || strcmp (uri, surface->filename)) {
		CleanupSurface ();

		if (!CreateSurface (uri)) {
			printf ("failed to create surface %s\n", uri);
			g_free (uri);
			Invalidate ();
			return;
		}
	}

	g_free (uri);
	
	updating_size_from_media = true;
	
	if (use_media_width) {
		Value *height = GetValueNoDefault (FrameworkElement::HeightProperty);

		if (!use_media_height)
			SetWidth ((double) surface->width * height->AsDouble () / (double) surface->height);
		else
			SetWidth ((double) surface->width);
	}
	
	if (use_media_height) {
		Value *width = GetValueNoDefault (FrameworkElement::WidthProperty);

		if (!use_media_width)
			SetHeight ((double) surface->height * width->AsDouble () / (double) surface->width);
		else
			SetHeight ((double) surface->height);
	}
	
	updating_size_from_media = false;
	
	if (brush) {
		// FIXME: this is wrong, we probably need to set the
		// property, or use some other mechanism, but this is
		// gross.
		PropertyChangedEventArgs args (ImageBrush::DownloadProgressProperty, NULL, 
					       brush->GetValue (ImageBrush::DownloadProgressProperty));
		
		brush->OnPropertyChanged (&args);
	} else
		Invalidate ();
}

void
Image::DownloaderFailed (EventArgs *args)
{
	ErrorEventArgs *err = NULL;
	
	if (args && args->GetObjectType () == Type::ERROREVENTARGS)
		err = (ErrorEventArgs *) args;
	
	Emit (ImageFailedEvent, new ImageErrorEventArgs (err ? err->error_message : NULL));
	
	Invalidate ();
}


#ifdef WORDS_BIGENDIAN
#define set_pixel_bgra(pixel,index,b,g,r,a) \
	G_STMT_START { \
		((unsigned char *)(pixel))[index]   = a; \
		((unsigned char *)(pixel))[index+1] = r; \
		((unsigned char *)(pixel))[index+2] = g; \
		((unsigned char *)(pixel))[index+3] = b; \
	} G_STMT_END
#define get_pixel_bgr_p(p,b,g,r) \
	G_STMT_START { \
		r = *(p);   \
		g = *(p+1); \
		b = *(p+2); \
	} G_STMT_END
#else
#define set_pixel_bgra(pixel,index,b,g,r,a) \
	G_STMT_START { \
		((unsigned char *)(pixel))[index]   = b; \
		((unsigned char *)(pixel))[index+1] = g; \
		((unsigned char *)(pixel))[index+2] = r; \
		((unsigned char *)(pixel))[index+3] = a; \
	} G_STMT_END
#define get_pixel_bgr_p(p,b,g,r) \
	G_STMT_START { \
		b = *(p);   \
		g = *(p+1); \
		r = *(p+2); \
	} G_STMT_END
#endif
#define get_pixel_bgra(color, b, g, r, a) \
	G_STMT_START { \
		a = ((color & 0xff000000) >> 24); \
		r = ((color & 0x00ff0000) >> 16); \
		g = ((color & 0x0000ff00) >> 8); \
		b = (color & 0x000000ff); \
	} G_STMT_END
#define get_pixel_bgr(color, b, g, r) \
	G_STMT_START { \
		r = ((color & 0x00ff0000) >> 16); \
		g = ((color & 0x0000ff00) >> 8); \
		b = (color & 0x000000ff); \
	} G_STMT_END
#include "alpha-premul-table.inc"

//
// Expands RGB to ARGB allocating new buffer for it.
//
static guchar*
expand_rgb_to_argb (GdkPixbuf *pixbuf, int *stride)
{
	guchar *pb_pixels = gdk_pixbuf_get_pixels (pixbuf);
	guchar *p;
	int w = gdk_pixbuf_get_width (pixbuf);
	int h = gdk_pixbuf_get_height (pixbuf);
	*stride = w * 4;
	guchar *data = (guchar *) g_malloc (*stride * h);
	guchar *out;

	for (int y = 0; y < h; y ++) {
		p = pb_pixels + y * gdk_pixbuf_get_rowstride (pixbuf);
		out = data + y * (*stride);
		if (false && gdk_pixbuf_get_rowstride (pixbuf) % 4 == 0) {
			for (int x = 0; x < w; x ++) {
				guint32 color = *(guint32*)p;
				guchar r, g, b;

				get_pixel_bgr (color, b, g, r);
				set_pixel_bgra (out, 0, r, g, b, 255);

				p += 3;
				out += 4;
			}
		}
		else {
			for (int x = 0; x < w; x ++) {
				guchar r, g, b;

				get_pixel_bgr_p (p, b, g, r);
				set_pixel_bgra (out, 0, r, g, b, 255);

				p += 3;
				out += 4;
			}
		}
	}

	return data;
}

//
// Converts RGBA unmultiplied alpha to ARGB pre-multiplied alpha.
//
static void
unmultiply_rgba_in_place (GdkPixbuf *pixbuf)
{
	guchar *pb_pixels = gdk_pixbuf_get_pixels (pixbuf);
	guchar *p;
	int w = gdk_pixbuf_get_width (pixbuf);
	int h = gdk_pixbuf_get_height (pixbuf);

	for (int y = 0; y < h; y ++) {
		p = pb_pixels + y * gdk_pixbuf_get_rowstride (pixbuf);
		for (int x = 0; x < w; x ++) {
			guint32 color = *(guint32*)p;
			guchar r, g, b, a;

			get_pixel_bgra (color, b, g, r, a);

			/* pre-multipled alpha */
			if (a == 0) {
				r = g = b = 0;
			}
			else if (a < 255) {
				r = pre_multiplied_table [r][a];
				g = pre_multiplied_table [g][a];
				b = pre_multiplied_table [b][a];
			}

			/* store it back, swapping red and blue */
			set_pixel_bgra (p, 0, r, g, b, a);

			p += 4;
		}
	}
}

bool
Image::CreateSurface (const char *uri)
{
	if (surface) {
		// image surface already created
		return true;
	}

	CleanupPattern ();

	if (!surface_cache)
		surface_cache = g_hash_table_new (g_str_hash, g_str_equal);
	
	if (uri == NULL || !(surface = (CachedSurface *) g_hash_table_lookup (surface_cache, uri))) {
		GdkPixbuf *pixbuf = NULL;
		char *msg;
		
		if (loader == NULL) {
			guchar buf[4096];
			ssize_t n;
			char *msg;
			char *filename;
			int fd;
                
			filename = downloader->GetDownloadedFilename (part_name);
			if (!filename) {
				msg = g_strdup_printf ("Failed to load image %s", part_name);
				Emit (ImageFailedEvent, new ImageErrorEventArgs (msg));
				return false;
			}

			loader = gdk_pixbuf_loader_new ();

			if ((fd = open (filename, O_RDONLY)) == -1) {
				gdk_pixbuf_loader_close (GDK_PIXBUF_LOADER (loader), NULL);
				g_object_unref (loader);
				msg = g_strdup_printf ("Failed to load image %s: %s", filename, g_strerror (errno));
				Emit (ImageFailedEvent, new ImageErrorEventArgs (msg));
				return false;
			}

			do {
				do {
					n = read (fd, buf, sizeof (buf));
				} while (n == -1 && errno == EINTR);

				if (n == -1)
					break;

				gdk_pixbuf_loader_write (GDK_PIXBUF_LOADER (loader), buf, n, &loader_err);
			} while (n > 0 && !loader_err);

			close (fd);
		}

		gdk_pixbuf_loader_close (GDK_PIXBUF_LOADER (loader), loader_err ? NULL : &loader_err);
		
		if (!(pixbuf = gdk_pixbuf_loader_get_pixbuf (GDK_PIXBUF_LOADER (loader)))) {
			g_object_unref (loader);
			if (loader_err && loader_err->message)
				msg = g_strdup_printf ("Failed to load image %s: %s", uri, loader_err->message);
			else
				msg = g_strdup_printf ("Failed to load image %s", uri);
			
			Emit (ImageFailedEvent, new ImageErrorEventArgs (msg));
			
			if (loader_err) {
				g_error_free (loader_err);
				loader_err = NULL;
			}

			return false;
		} else if (loader_err) {
			g_error_free (loader_err);
			loader_err = NULL;
		}
		g_object_ref (pixbuf);
		g_object_unref (loader);
		loader = NULL;
		
		surface = g_new0 (CachedSurface, 1);
		
		surface->ref_count = 1;
		surface->filename = g_strdup (uri);
		surface->height = gdk_pixbuf_get_height (pixbuf);
		surface->width = gdk_pixbuf_get_width (pixbuf);
		
		bool has_alpha = gdk_pixbuf_get_n_channels (pixbuf) == 4;
		guchar *data;
		int stride;
		
		if (has_alpha) {
			surface->backing_pixbuf = pixbuf;
			surface->backing_data = NULL;
			unmultiply_rgba_in_place (pixbuf);
			stride = gdk_pixbuf_get_rowstride (pixbuf);
			data = gdk_pixbuf_get_pixels (pixbuf);
		} else {
			surface->backing_pixbuf = NULL;
			surface->backing_data = expand_rgb_to_argb (pixbuf, &stride);
			data = surface->backing_data;
			g_object_unref (pixbuf);
		}

		surface->cairo = cairo_image_surface_create_for_data (data,
#if USE_OPT_RGB24
								      has_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
#else
								      CAIRO_FORMAT_ARGB32,
#endif
								      surface->width,
								      surface->height,
								      stride);

#if USE_OPT_RGB24
		surface->has_alpha = has_alpha;
#endif
		if (surface->filename != NULL)
			g_hash_table_insert (surface_cache, surface->filename, surface);
	} else {
		surface->ref_count++;
	}

	return true;
}

void
Image::size_notify (gint64 size, gpointer data)
{
	// Do something with it?
	// if size == -1, we do not know the size of the file, can happen
	// if the server does not return a Content-Length header
	//printf ("The image size is %lld\n", size);
}

void
Image::pixbuf_write (void *buf, gint32 offset, gint32 n, gpointer data)
{
	((Image *) data)->PixbufWrite (buf, offset, n);
}

void
Image::Render (cairo_t *cr, Region *region)
{
	if (!surface)
		return;
	
	if (create_xlib_surface && !surface->xlib_surface_created) {
		surface->xlib_surface_created = true;
		
		cairo_surface_t *xlib_surface = image_brush_create_similar (cr, surface->width, surface->height);
		cairo_t *cr = cairo_create (xlib_surface);

		cairo_set_source_surface (cr, surface->cairo, 0, 0);

		//cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);

		cairo_paint (cr);
		cairo_destroy (cr);

		cairo_surface_destroy (surface->cairo);

		if (surface->backing_pixbuf) {
			g_object_unref (surface->backing_pixbuf);
			surface->backing_pixbuf = NULL;
		}

		if (surface->backing_data) {
			g_free (surface->backing_data);
			surface->backing_data =NULL;
		}

		surface->cairo = xlib_surface;
	}

	cairo_save (cr);

	Stretch stretch = GetStretch ();
	double h = GetHeight ();
	double w = GetWidth ();
	
	if (!pattern)
		pattern = cairo_pattern_create_for_surface (surface->cairo);
	
	cairo_matrix_t matrix;
	
	image_brush_compute_pattern_matrix (&matrix, w, h, surface->width, surface->height, stretch, 
					    AlignmentXCenter, AlignmentYCenter, NULL, NULL);
	
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_set_source (cr, pattern);

	cairo_set_matrix (cr, &absolute_xform);
	
	cairo_rectangle (cr, 0, 0, w, h);
	cairo_fill (cr);

	cairo_restore (cr);
}

Point
Image::GetTransformOrigin ()
{
	Point *user_xform_origin = GetRenderTransformOrigin ();
	
	return Point (GetWidth () * user_xform_origin->x, 
		      GetHeight () * user_xform_origin->y);
}

Rect
Image::GetCoverageBounds ()
{
	Stretch stretch = GetStretch ();

	if (surface && !surface->has_alpha
	    && ((GetImageWidth () == GetBounds ().width
		 && GetImageHeight () == GetBounds ().height)
		|| (stretch == StretchFill || stretch == StretchUniformToFill)))
		return bounds;

	return Rect ();
}

cairo_surface_t *
Image::GetCairoSurface ()
{
	return surface ? surface->cairo : NULL;
}

void
Image::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property == FrameworkElement::HeightProperty) {
		if (!updating_size_from_media)
			use_media_height = args->new_value == NULL;
	} else if (args->property == FrameworkElement::WidthProperty) {
		if (!updating_size_from_media)
			use_media_width = args->new_value == NULL;
	}
	
	if (args->property->GetOwnerType() != Type::IMAGE) {
		MediaBase::OnPropertyChanged (args);
		return;
	}
	
	// we need to notify attachees if our DownloadProgress changed.
	NotifyListenersOfPropertyChange (args);
}

bool
Image::InsideObject (cairo_t *cr, double x, double y)
{
	if (!surface)
		return false;

	return FrameworkElement::InsideObject (cr, x, y);
}


//
// MediaAttributeCollection
//

MediaAttribute *
MediaAttributeCollection::GetItemByName (const char *name)
{
	MediaAttribute *attr;
	const char *value;
	
	for (guint i = 0; i < array->len; i++) {
		attr = ((Value *) array->pdata[i])->AsMediaAttribute ();
		if (!(value = attr->GetName ()))
			continue;
		
		if (!strcmp (value, name))
			return attr;
	}
	
	return NULL;
}


//
// TimelineMarkerCollection
//

int
TimelineMarkerCollection::Add (Value *value)
{
	TimelineMarker *marker, *cur;
	
	marker = value->AsTimelineMarker ();
	
	for (guint i = 0; i < array->len; i++) {
		cur = ((Value *) array->pdata[i])->AsTimelineMarker ();
		if (cur->GetTime () >= marker->GetTime ()) {
			DependencyObjectCollection::Insert (i, value);
			return i;
		}
	}
	
	return DependencyObjectCollection::Insert (array->len, value) ? array->len - 1 : -1;
}

bool
TimelineMarkerCollection::Insert (int index, Value *value)
{
	return Add (value) != -1;
}


//
// MarkerReachedEventArgs
//

MarkerReachedEventArgs::MarkerReachedEventArgs (TimelineMarker *marker)
{
	this->marker = marker;
	marker->ref ();
}

MarkerReachedEventArgs::~MarkerReachedEventArgs ()
{
	marker->unref ();
}
