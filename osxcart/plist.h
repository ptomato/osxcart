#ifndef __OSXCART_PLIST_H__
#define __OSXCART_PLIST_H__

/* Copyright 2009 P. F. Chimento
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
 *
 * The different error codes which can be thrown in the #PLIST_ERROR domain.
 */
typedef enum {
	PLIST_ERROR_FAILED,            /* Generic error */
	PLIST_ERROR_BAD_VERSION,       /* The plist was an incompatible version */
	PLIST_ERROR_UNEXPECTED_OBJECT, /* An object was out of place in the plist */
	PLIST_ERROR_EXTRANEOUS_KEY,    /* A <key> element was encountered outside a
	                                  <dict> object */
	PLIST_ERROR_MISSING_KEY,       /* A <dict> object was missing a <key> */
	PLIST_ERROR_BAD_DATE,          /* A <date> object contained incorrect
	                                  formatting */
	PLIST_ERROR_NO_ELEMENTS        /* The plist was empty */
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
