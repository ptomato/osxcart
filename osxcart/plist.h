#ifndef __OSXCART_PLIST_H__
#define __OSXCART_PLIST_H__

/* Copyright 2009, 2011 P. F. Chimento
This file is part of Osxcart.

Osxcart is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) any later 
version.

Osxcart is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along 
with Osxcart.  If not, see <http://www.gnu.org/licenses/>. */

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

/**
 * PlistError:
 * @PLIST_ERROR_FAILED: A generic error.
 * @PLIST_ERROR_BAD_VERSION: The plist was an incompatible version.
 * @PLIST_ERROR_UNEXPECTED_OBJECT: An object was out of place in the plist.
 * @PLIST_ERROR_EXTRANEOUS_KEY: A <code>&lt;key&gt;</code> element was
 * encountered outside a <code>&lt;dict&gt;</code> object.
 * @PLIST_ERROR_MISSING_KEY: A <code>&lt;dict&gt;</code> object was missing a
 * <code>&lt;key&gt;</code> element.
 * @PLIST_ERROR_BAD_DATE: A <code>&lt;date&gt;</code> object contained incorrect
 * formatting.
 * @PLIST_ERROR_NO_ELEMENTS: The plist was empty.
 *
 * The different error codes which can be thrown in the #PLIST_ERROR domain.
 */
typedef enum {
	PLIST_ERROR_FAILED,
	PLIST_ERROR_BAD_VERSION,
	PLIST_ERROR_UNEXPECTED_OBJECT,
	PLIST_ERROR_EXTRANEOUS_KEY,
	PLIST_ERROR_MISSING_KEY,
	PLIST_ERROR_BAD_DATE,
	PLIST_ERROR_NO_ELEMENTS
} PlistError;

/**
 * PLIST_ERROR:
 *
 * The domain of errors raised by property list processing in Osxcart.
 */
#define PLIST_ERROR plist_error_quark()

GQuark plist_error_quark(void);
GVariant *plist_object_lookup(GVariant *tree, ...);
GVariant *plist_read(const gchar *filename, GError **error);
GVariant *plist_read_from_string(const gchar *string, GError **error);
gboolean plist_write(GVariant *plist, const gchar *filename, GError **error);
gchar *plist_write_to_string(GVariant *plist);

G_END_DECLS

#endif /* __OSXCART_PLIST_H__ */
