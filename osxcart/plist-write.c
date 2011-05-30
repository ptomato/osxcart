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

#include <string.h>
#include <glib.h>
#include <osxcart/plist.h>
#include "init.h"

/* plist-write.c - Simple recursive plist writer */

typedef struct {
	GString *buffer;
	gint num_indents;
	GVariant *dict;
} PlistDumpContext;

/* Forward declarations */
static void plist_dump(GVariant *object, PlistDumpContext *context);

/* Output a key and its corresponding object */
static void
dump_key_value_pair(const gchar *key, PlistDumpContext *context)
{
	GVariant *dict = context->dict;
	gchar *tabs = g_strnfill(context->num_indents, '\t');
	GVariant *value = g_variant_lookup_value(dict, key, NULL);
	
	g_string_append_printf(context->buffer, "%s<key>%s</key>\n", tabs, key);
	g_free(tabs);
	plist_dump(value, context);
	context->dict = dict;
}

/* Output an object; recurse if the object is a container */
static void
plist_dump(GVariant *object, PlistDumpContext *context)
{
	gchar *tempstr, *tabs;

	if(object == NULL)
		return;

	tabs = g_strnfill(context->num_indents, '\t');
	g_string_append(context->buffer, tabs);

	if(g_variant_is_of_type(object, G_VARIANT_TYPE_BOOLEAN))
		g_string_append(context->buffer, g_variant_get_boolean(object)? "<true/>\n" : "<false/>\n");
	
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE_DOUBLE))
		g_string_append_printf(context->buffer, "<real>%.14f</real>\n", g_variant_get_double(object));
	
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE_INT32))
		g_string_append_printf(context->buffer, "<integer>%d</integer>\n", g_variant_get_int32(object));
		
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE_STRING)) {
		gsize length;
		const char *string = g_variant_get_string(object, &length);
		if(string == NULL || length == 0)
			g_string_append(context->buffer, "<string></string>\n");
		else {
			tempstr = g_markup_escape_text(string, length);
			g_string_append_printf(context->buffer, "<string>%s</string>\n", tempstr);
			g_free(tempstr);
		}
	}
		
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE("(xx)"))) {
		GTimeVal timeval;
		g_variant_get(object, "(xx)", &timeval.tv_sec, &timeval.tv_usec);
		tempstr = g_time_val_to_iso8601(&timeval);
		g_string_append_printf(context->buffer, "<date>%s</date>\n", tempstr);
		g_free(tempstr);
	}

	else if(g_variant_is_of_type(object, G_VARIANT_TYPE("av"))) {
		GVariantIter *iter;
		GVariant *value;
		g_variant_get(object, "av", &iter);
	
		if(g_variant_iter_n_children(iter) > 0) {
			g_string_append(context->buffer, "<array>\n");
			context->num_indents++;
			while(g_variant_iter_loop(iter, "v", &value))
				plist_dump(value, context);
			context->num_indents--;
			g_string_append_printf(context->buffer, "%s</array>\n", tabs);
		} else
			g_string_append(context->buffer, "<array/>\n");
		g_variant_iter_free(iter);
	}
	
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE("a{sv}"))) {
		GVariantIter *iter;
		GList *keys = NULL;
		const char *key;

		/* Put all the keys into a list so we can dump them alphabetically */
		g_variant_get(object, "a{sv}", &iter);
		while(g_variant_iter_loop(iter, "{sv}", &key, NULL))
			keys = g_list_prepend(keys, g_strdup(key));
		g_variant_iter_free(iter);

		if(keys != NULL) {
			keys = g_list_sort(keys, (GCompareFunc)strcmp);
						
			g_string_append(context->buffer, "<dict>\n");
			context->num_indents++;
			context->dict = object;
			g_list_foreach(keys, (GFunc)dump_key_value_pair, context);
			context->num_indents--;
			context->dict = NULL;
			g_string_append_printf(context->buffer, "%s</dict>\n", tabs);
		} else
			g_string_append(context->buffer, "<dict/>\n");

		g_list_foreach(keys, (GFunc)g_free, NULL);
		g_list_free(keys);
	}
	
	else if(g_variant_is_of_type(object, G_VARIANT_TYPE("ay"))) {
		gsize length = g_variant_n_children(object);
		unsigned char *data = g_new0(unsigned char, length);
		unsigned char *ptr = data;
		GVariantIter *iter;
		
		g_variant_get(object, "ay", &iter);
		while(g_variant_iter_loop(iter, "y", ptr++))
			/*pass*/ ;
		g_variant_iter_free(iter);
		tempstr = g_base64_encode(data, length);
		g_string_append_printf(context->buffer, "<data>%s</data>\n", tempstr);
		g_free(tempstr);
	}
	g_free(tabs);
}

/**
 * plist_write:
 * @plist: A property list object.
 * @filename: The filename to write to.
 * @error: Return location for an error, or %NULL.
 *
 * Writes the property list @plist to a file in XML format. If @filename exists,
 * it will be overwritten.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if not, in which case
 * @error is set.
 */
gboolean
plist_write(GVariant *plist, const gchar *filename, GError **error)
{
	gchar *string;
	gboolean retval;

	osxcart_init();
	
	g_return_val_if_fail(plist != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	string = plist_write_to_string(plist);
	retval = g_file_set_contents(filename, string, -1, error);
	g_free(string);
	return retval;
}

/**
 * plist_write_to_string:
 * @plist: A property list object.
 * 
 * Writes the property list @plist to a string in XML format.
 *
 * Returns: a newly-allocated string containing an XML property list. The string
 * must be freed with <link linkend="glib-Memory-Allocation">g_free()</link> 
 * when you are done with it.
 */
gchar *
plist_write_to_string(GVariant *plist)
{
	PlistDumpContext *context;
	GString *buffer;

	osxcart_init();
	
	g_return_val_if_fail(plist != NULL, NULL);

	buffer = g_string_new(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" "
		"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
		"<plist version=\"1.0\">\n");
	context = g_slice_new0(PlistDumpContext);	
	context->buffer = buffer;
	plist_dump(plist, context);
	g_slice_free(PlistDumpContext, context);
	g_string_append(buffer, "</plist>\n");
	return g_string_free(buffer, FALSE);
}
