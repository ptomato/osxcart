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

#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "init.h"

/* plist-parser.c - This is a plist parser implemented in GLib's GMarkup SAX
parser API. */

typedef struct {
	GVariantBuilder *builder; /* Builder for adding objects, if in dict_parser
							   or array_parser */
	gchar *key;	              /* key of current object, if in dict_parser */
	GVariant *retval;         /* toplevel object, set during bare_parser */
} ParseData;

/* Forward declarations */
typedef void ParserStartFunc(GMarkupParseContext *context, const char *element_name, const char **attribute_names, const char **attribute_values, gpointer user_data, GError **error);
typedef void ParserEndFunc(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **error);
typedef void ParserTextFunc(GMarkupParseContext *context, const char *text, gsize text_len, gpointer user_data, GError **error);

static ParserStartFunc bare_start, dict_start, array_start, plist_start;
static ParserEndFunc bare_end, dict_end, array_end, plist_end;
static ParserTextFunc bare_text, dict_text, array_text;

/* Callback functions to deal with the opening and closing tags and the content
of XML elements representing plist objects not inside a container */
static GMarkupParser bare_parser = { 
	bare_start, bare_end, bare_text, NULL, NULL
};

/* Callback functions to deal with XML elements representing plist objects
inside a <dict> container */
static GMarkupParser dict_parser = {
	dict_start, dict_end, dict_text, NULL, NULL
};

/* Callback functions to deal with XML elements representing plist objects
inside an <array> container */
static GMarkupParser array_parser = {
	array_start, array_end, array_text, NULL, NULL
};

/* Callback functions to deal with the <plist> root element opening and closing
tags */
static GMarkupParser plist_parser = { 
	plist_start, plist_end, NULL, NULL, NULL
};

/* Custom string equality function, for typing convenience */
static inline gboolean 
str_eq(const gchar *s1, const gchar *s2)
{
	return (g_ascii_strcasecmp(s1, s2) == 0);
}

/* Callback for processing an opening XML element. This is the outside element 
 of the plist. It could be a regular element, or a container like array or dict.
 By the time the element is closed, data->retval should point to a GVariant
 containing the element. We do not know what the content of the element is, so
 it is not created until fill_object() (for non-containers) or when the
 GVariantBuilder is closed (for containers); the exception is <true> and
 <false> elements, whose content is already obvious. */
static void
bare_start(GMarkupParseContext *context, const char *element_name, const char **attribute_names, const char **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
		
	if(data->retval != NULL) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_UNEXPECTED_OBJECT, _("Unexpected object <%s>; subsequent object ought to be enclosed in an <array> or <dict>"), element_name);
		return;
	}
	
	/* <true> - assign val here */
	if(str_eq(element_name, "true"))
		data->retval = g_variant_new_boolean(TRUE);
	
	/* <false> - assign val here */
	if(str_eq(element_name, "false"))
		data->retval = g_variant_new_boolean(FALSE);
	
	/* <array> - create a new VariantBuilder and change to state array_parser */
	if(str_eq(element_name, "array")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
		g_markup_parse_context_push(context, &array_parser, new_data);
	}
	
	/* <dict> - create a new VariantBuilder and change to state dict_parser */
	else if(str_eq(element_name, "dict")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
		g_markup_parse_context_push(context, &dict_parser, new_data);
	}

	/* <key> - invalid if not in a <dict> */
	else if(str_eq(element_name, "key"))
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_EXTRANEOUS_KEY, _("<key> element found outside of <dict>"));
	
	/* For other elements, we do not know what the content of the element is, so
	 no variant is created until fill_object(). */
}	

/* Callback for processing content of XML elements */
static void
bare_text(GMarkupParseContext *context, const char *text, gsize text_len, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;

	const gchar *name = g_markup_parse_context_get_element(context);
		
	/* There should be no text in <true> or <false> */
	if(str_eq(name, "true") || str_eq(name, "false"))
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, _("<%s> should have no content, but found '%s'"), name, text);

	/* <real> - this assumes that property lists do NOT contain localized 
	representation of numbers */
	else if(str_eq(name, "real"))
		data->retval = g_variant_new_double(g_ascii_strtod(text, NULL));
	
	else if(str_eq(name, "integer"))
		data->retval = g_variant_new_int32(atoi(text));
	
	else if(str_eq(name, "string"))
		data->retval = g_variant_new_string(text); /* copies string */

	else if(str_eq(name, "date")) {
		GTimeVal timeval;
		if(!g_time_val_from_iso8601(text, &timeval))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_DATE, _("Could not parse date '%s'"), text);
		data->retval = g_variant_new_parsed("(%x, %x)", timeval.tv_sec, timeval.tv_usec);
	}
	
	else if(str_eq(name, "data")) {
		gsize buflen = 0;
		guchar *buffer = g_base64_decode(text, &buflen);
		/* SUCKY DEBIAN use g_variant_new_bytestring() */
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));
		int count;
		for(count = 0; count < buflen; count++)
			g_variant_builder_add(&builder, "y", buffer[count]);
		data->retval = g_variant_builder_end(&builder);
		g_free(buffer);
	}

	else if(str_eq(name, "plist"))
		return; /* Ignore white space in the outer element */

	else
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Unknown object <%s>"), name);
		
	return;
}

/* Callback for processing closing XML elements */
static void
bare_end(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **error)
{	
	ParseData *data = (ParseData *)user_data;
	
	/* </array>, </dict> - close the variant builder */
	if(str_eq(element_name, "array") || str_eq(element_name, "dict")) {
		ParseData *sub_data = g_markup_parse_context_pop(context);
		data->retval = g_variant_builder_end(sub_data->builder);
		g_variant_builder_unref(sub_data->builder);
		g_slice_free(ParseData, sub_data);
		return;
	}
	
	/* If the element is still NULL, that means that the tag we have been 
	handling wasn't recognized. */
	if(data->retval == NULL)
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Unknown object <%s>"), element_name);
	
	/* other element - do nothing */
}

/* Common code for element start inside a container context (array or dict) */
static void
container_start(GMarkupParseContext *context, const char *element_name, ParseData *data)
{
	/* <array> - open a new level inside this GVariantBuilder and push the array
	 parser context onto the stack */
	if(str_eq(element_name, "array")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
		g_markup_parse_context_push(context, &array_parser, new_data);
	}
	
	/* <dict> - open a new level inside this GVariantBuilder and push the dict
	 parser context onto the stack */
	else if(str_eq(element_name, "dict")) {
		ParseData *new_data = g_slice_new0(ParseData);
		new_data->builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
		g_markup_parse_context_push(context, &dict_parser, new_data);
	}
}

static void
array_start(GMarkupParseContext *context, const char *element_name, const char **attribute_names, const char **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
		
	/* <true> - assign val here */
	if(str_eq(element_name, "true"))
		g_variant_builder_add(data->builder, "v", g_variant_new_boolean(TRUE));
	
	/* <false> - assign val here */
	else if(str_eq(element_name, "false"))
		g_variant_builder_add(data->builder, "v", g_variant_new_boolean(FALSE));
	
	/* <key> - invalid if not in a <dict> */
	else if(str_eq(element_name, "key"))
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_EXTRANEOUS_KEY, _("<key> element found outside of <dict>"));
	
	else
		container_start(context, element_name, data);
}

/* Common code for element content in array and dict contexts */
static void
array_text(GMarkupParseContext *context, const char *text, gsize text_len, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	
	const gchar *name = g_markup_parse_context_get_element(context);
		
	/* There should be no text in <true> or <false> */
	if(str_eq(name, "true") || str_eq(name, "false"))
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, _("<%s> should have no content, but found '%s'"), name, text);
	
	/* <real> - this assumes that property lists do NOT contain localized 
	 representation of numbers */
	else if(str_eq(name, "real"))
		g_variant_builder_add(data->builder, "v", g_variant_new_double(g_ascii_strtod(text, NULL)));
	
	else if(str_eq(name, "integer"))
		g_variant_builder_add(data->builder, "v", g_variant_new_int32(atoi(text)));
	
	else if(str_eq(name, "string"))
		g_variant_builder_add(data->builder, "v", g_variant_new_string(text)); /* copies string */
	
	else if(str_eq(name, "date")) {
		GTimeVal timeval;
		if(!g_time_val_from_iso8601(text, &timeval))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_DATE, _("Could not parse date '%s'"), text);
		GVariant *date_variant = g_variant_new_parsed("(%x,%x)", timeval.tv_sec, timeval.tv_usec);
		g_variant_builder_add(data->builder, "v", date_variant);
	}
	
	else if(str_eq(name, "data")) {
		gsize buflen = 0;
		guchar *buffer = g_base64_decode(text, &buflen);
		/* SUCKY DEBIAN use g_variant_new_bytestring() */
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));
		int count;
		for(count = 0; count < buflen; count++)
			g_variant_builder_add(&builder, "y", buffer[count]);
		GVariant *data_array = g_variant_builder_end(&builder);
		g_variant_builder_add(data->builder, "v", data_array);
		g_free(buffer);
	}
	
	/* else - just ignore text, because it could be whitespace */
	
	return;
}

static void
array_end(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	
	/* </array>, </dict> - close the variant builder */
	if(str_eq(element_name, "array") || str_eq(element_name, "dict")) {
		ParseData *sub_data = g_markup_parse_context_pop(context);
		g_variant_builder_add_value(data->builder, g_variant_builder_end(sub_data->builder));
		g_variant_builder_unref(sub_data->builder);
		g_slice_free(ParseData, sub_data);
	}
	
	/* other element - do nothing */
}

static void
dict_start(GMarkupParseContext *context, const char *element_name, const char **attribute_names, const char **attribute_values, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
		
	if(data->key == NULL && !str_eq(element_name, "key")) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_MISSING_KEY, _("Missing <key> for object <%s> in <dict>"), element_name);
		return;
	}
	
	/* <true> - assign val here */
	else if(str_eq(element_name, "true"))
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_new_boolean(TRUE));
	
	/* <false> - assign val here */
	else if(str_eq(element_name, "false"))
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_new_boolean(FALSE));
	
	else
		container_start(context, element_name, data);
}

/* Callback for processing content of XML elements */
static void
dict_text(GMarkupParseContext *context, const char *text, gsize text_len, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;
	
	const gchar *name = g_markup_parse_context_get_element(context);
		
	/* key */
	if(str_eq(name, "key"))
		data->key = g_strdup(text);
	
	/* There should be no text in <true> or <false> */
	else if(str_eq(name, "true") || str_eq(name, "false"))
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, _("<%s> should have no content, but found '%s'"), name, text);
	
	/* <real> - this assumes that property lists do NOT contain localized 
	 representation of numbers */
	else if(str_eq(name, "real"))
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_new_double(g_ascii_strtod(text, NULL)));
	
	else if(str_eq(name, "integer"))
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_new_int32(atoi(text)));
	
	else if(str_eq(name, "string"))
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_new_string(text)); /* copies string */
	
	else if(str_eq(name, "date")) {
		GTimeVal timeval;
		if(!g_time_val_from_iso8601(text, &timeval))
			g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_DATE, _("Could not parse date '%s'"), text);
		GVariant *date_variant = g_variant_new_parsed("(%x,%x)", timeval.tv_sec, timeval.tv_usec);
		g_variant_builder_add(data->builder, "{sv}", data->key, date_variant);
	}
	
	else if(str_eq(name, "data")) {
		gsize buflen = 0;
		guchar *buffer = g_base64_decode(text, &buflen);
		/* SUCKY DEBIAN use g_variant_new_bytestring() */
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));
		int count;
		for(count = 0; count < buflen; count++)
			g_variant_builder_add(&builder, "y", buffer[count]);
		GVariant *data_array = g_variant_builder_end(&builder);
		g_variant_builder_add(data->builder, "{sv}", data->key, data_array);
		g_free(buffer);
	}
	
	/* else - just ignore text, because it could be whitespace */
}

static void
dict_end(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **error)
{
	ParseData *data = (ParseData *)user_data;

	/* </array>, </dict> - close the variant builder */
	if(str_eq(element_name, "dict") || str_eq(element_name, "array")) {
		ParseData *sub_data = g_markup_parse_context_pop(context);
		g_variant_builder_add(data->builder, "{sv}", data->key, g_variant_builder_end(sub_data->builder));
		g_variant_builder_unref(sub_data->builder);
		g_free(sub_data->key);
		g_slice_free(ParseData, sub_data);
	}
	
	/* other element - do nothing */
}

/* Callback for processing opening tag of root <plist> element */
static void
plist_start(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseData *parse_data;
	const char *version_string;
	
	/* Check the root element of the plist. Make sure it is named <plist>, and that
	 it is version 1.0 */
	if(!str_eq(element_name, "plist")) {
		g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("<plist> root element not found; got <%s> instead"), element_name);
		return;
	}
	
	if(!g_markup_collect_attributes(element_name, attribute_names, attribute_values, error,
									G_MARKUP_COLLECT_STRING, "version", &version_string,
									G_MARKUP_COLLECT_INVALID))
		return;
	/* Don't free version_string */
	
	if(!str_eq(version_string, "1.0")) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_BAD_VERSION, _("Unsupported plist version '%s'"), version_string);
		return;
	}
	
	parse_data = g_slice_new0(ParseData);
	g_markup_parse_context_push(context, &bare_parser, parse_data);
}

/* Callback for processing closing </plist> tag */
static void
plist_end(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	GVariant **data = (GVariant **)user_data;
	
	ParseData *parse_data = g_markup_parse_context_pop(context);
	if(parse_data->retval == NULL) {
		g_set_error(error, PLIST_ERROR, PLIST_ERROR_NO_ELEMENTS, _("No objects found within <plist> root element"));
		return;
	}
	*data = parse_data->retval;
}

/**
 * plist_read:
 * @filename: The path to a file containing a property list in XML format.
 * @error: Return location for an error, or %NULL.
 *
 * Reads a property list in XML format from @filename and returns a #PlistObject 
 * representing the property list.
 *
 * Returns: the property list, or %NULL if an error occurred, in which case
 * @error is set. The property list must be freed with plist_object_free() after
 * use.
 */
GVariant *
plist_read(const gchar *filename, GError **error)
{
	gchar *contents;
	GVariant *retval;

	osxcart_init();
	
	g_return_val_if_fail(filename != NULL, NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);

	if(!g_file_get_contents(filename, &contents, NULL, error))
		return NULL;
	retval = plist_read_from_string(contents, error);
	g_free(contents);
	return retval;
}

/**
 * plist_read_from_string:
 * @string: A string containing a property list in XML format.
 * @error: Return location for an error, or %NULL.
 *
 * Reads a property list in XML format from @string and returns a #PlistObject
 * representing the property list.
 *
 * Returns: the property list, or %NULL if an error occurred, in which case
 * @error is set. The property list must be freed with plist_object_free() after
 * use.
 */
GVariant *
plist_read_from_string(const gchar *string, GError **error)
{
	GMarkupParseContext *context;
	GVariant *plist = NULL;
	
	osxcart_init();

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);
	
	context = g_markup_parse_context_new(&plist_parser, G_MARKUP_PREFIX_ERROR_POSITION, &plist, NULL);
	if(!g_markup_parse_context_parse(context, string, -1, error) || !g_markup_parse_context_end_parse(context, error)) {
		g_markup_parse_context_free(context);
		return NULL;
	}
	g_markup_parse_context_free(context);
	return plist;
}
