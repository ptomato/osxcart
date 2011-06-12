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

#include <stdarg.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <osxcart/plist.h>
#include "init.h"

/**
 * SECTION:plist
 * @short_description: Tools for manipulating property lists
 * @stability: Unstable
 * @include: osxcart/plist.h
 *
 * Property lists are used in Mac OS X, NeXTSTEP, and GNUstep to store 
 * serialized objects. Mac OS X uses an XML format to store property lists in
 * files with the extension <quote>.plist</quote>. This module reads and writes
 * property lists in the XML format. For more information on the format, see the
 * <ulink 
 * link="http://developer.apple.com/documentation/Darwin/Reference/ManPages/man5/plist.5.html">
 * Apple developer documentation</ulink>.
 *
 * Instead of deserializing the property list into Core Foundation types as in
 * Mac OS X, the property list is represented using a hierarchical structure of
 * #GVariant<!---->s, lightweight objects that can contain any type of data.
 * Earlier versions of the Osxcart library used custom structures for this
 * purpose, but since the addition of #GVariant to GLib, this was changed in
 * order to avoid duplication of functionality.
 *
 * Most property list object types have a corresponding #GVariant data type, but
 * for example, dates are stored as a #GVariant tuple of int64 values
 * corresponding to the @tv_sec and @tv_usec fields of #GTimeVal. For
 * completeness, the data types are listed here:
 *
 * <informaltable>
 *   <tgroup cols='3'>
 *     <thead>
 *       <row>
 *         <entry>XML Element</entry>
 *         <entry>Core Foundation data type</entry>
 *         <entry>GVariant type symbol</entry>
 *       </row>
 *     </thead>
 *     <tbody>
 *       <row>
 *         <entry><code>true</code>, <code>false</code></entry>
 *         <entry><code>CFBoolean</code></entry>
 *         <entry><code>b</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>integer</code></entry>
 *         <entry><code>CFNumber</code></entry>
 *         <entry><code>i</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>real</code></entry>
 *         <entry><code>CFNumber</code></entry>
 *         <entry><code>d</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>string</code></entry>
 *         <entry><code>CFString</code></entry>
 *         <entry><code>s</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>date</code></entry>
 *         <entry><code>CFDate</code></entry>
 *         <entry><code>(xx)</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>data</code></entry>
 *         <entry><code>CFData</code></entry>
 *         <entry><code>ay</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>array</code></entry>
 *         <entry><code>CFArray</code></entry>
 *         <entry><code>av</code></entry>
 *       </row>
 *       <row>
 *         <entry><code>dict</code></entry>
 *         <entry><code>CFDictionary</code></entry>
 *         <entry><code>a{sv}</code></entry>
 *       </row>
 *     </tbody>
 *   </tgroup>
 * </informaltable>
 */

/**
 * plist_error_quark:
 *
 * The error domain for property list errors.
 *
 * Returns: (transfer none): The string <quote>plist-error-quark</quote> as a
 * <link linkend="GQuark">GQuark</link>.
 */
GQuark
plist_error_quark(void)
{
	osxcart_init();
	return g_quark_from_static_string("plist-error-quark");
}

/**
 * plist_object_lookup:
 * @tree: The root object of the plist
 * @Varargs: A path consisting of dictionary keys and array indices, terminated
 * by -1
 *
 * Convenience function for looking up an object that exists at a certain path
 * within the plist. The variable argument list can consist of either strings 
 * (dictionary keys, if the object at that point in the path is a dict) or 
 * integers (array indices, if the object at that point in the path is an 
 * array.) 
 * 
 * The variable argument list must be terminated by -1. 
 * 
 * For example, given the following plist: 
 * |[&lt;plist version="1.0"&gt; 
 * &lt;dict&gt; 
 *   &lt;key&gt;Array&lt;/key&gt; 
 *   &lt;array&gt; 
 *     &lt;integer&gt;1&lt;/integer&gt;
 *     &lt;string&gt;2&lt;/string&gt;
 *     &lt;real&gt;3.0&lt;/real&gt; 
 *   &lt;/array&gt; 
 *   &lt;key&gt;Dict&lt;/key&gt;
 *   &lt;dict&gt;
 *     &lt;key&gt;Integer&lt;/key&gt;
 *     &lt;integer&gt;1&lt;/integer&gt;
 *     &lt;key&gt;Real&lt;/key&gt;
 *     &lt;real&gt;2.0&lt;/real&gt;
 *     &lt;key&gt;String&lt;/key&gt;
 *     &lt;string&gt;3&lt;/string&gt;
 *   &lt;/dict&gt;
 * &lt;/plist&gt;]| 
 * then the following code: 
 * |[GVariant *obj1 = plist_object_lookup(plist, "Array", 0, -1); 
 * GVariant *obj2 = plist_object_lookup(plist, "Dict", "Integer", -1);]| 
 * will place in @obj1 and @obj2 two identical #GVariant<!---->s containing
 * the integer 1, although they will both point to two different spots in the
 * @plist tree. 
 * 
 * Returns: (transfer none): The requested #GVariant, or %NULL if the path did
 * not exist. The returned object is a pointer to the object within the original
 * @tree, and is not given an extra reference. Therefore, it should not be
 * unreferenced when you are done with it; conversely, if you want to keep it
 * around longer than the lifetime of @tree, you should reference it.
 */
GVariant *
plist_object_lookup(GVariant *tree, ...)
{
	g_return_val_if_fail(tree, NULL);
	
	va_list ap;
	gpointer arg;
	
	va_start(ap, tree);
	for(arg = va_arg(ap, gpointer); GPOINTER_TO_INT(arg) != -1; arg = va_arg(ap, gpointer)) {
		if(g_variant_is_of_type(tree, G_VARIANT_TYPE("a{sv}")))
			tree = g_variant_lookup_value(tree, (const gchar *)arg, NULL);
		else if(g_variant_is_of_type(tree, G_VARIANT_TYPE("av")))
			tree = g_variant_get_child_value(tree, GPOINTER_TO_UINT(arg));
		else {
			g_critical("%s: %s", __func__, _("Tried to look up a child of an "
				"object that wasn't a dict or array"));
			return tree;
		}
		/* Return NULL if one of the keys or indices wasn't found */
		if(tree == NULL)
			break;
	}
	va_end(ap);
	
	/* Unbox the final value if it is a basic type */
	if(g_variant_is_of_type(tree, G_VARIANT_TYPE_VARIANT))
		tree = g_variant_get_variant(tree);

	return tree;
}

