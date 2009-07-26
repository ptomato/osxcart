#include <string.h>
#include <glib.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <osxcart/rtf.h>
#include "rtf-deserialize.h"
#include "rtf-document.h"

/* Document destination functions */
static DocFunc doc_ansi, doc_footnote, doc_mac, doc_pc, doc_pca;
static DocParamFunc doc_ansicpg, doc_deff, doc_rtf;

extern const DestinationInfo colortbl_destination, field_destination,
                             fonttbl_destination, footnote_destination,
                             stylesheet_destination;

const ControlWord document_word_table[] = {
	DOCUMENT_TEXT_CONTROL_WORDS,
	{ "ansi", NO_PARAMETER, FALSE, doc_ansi },
	{ "ansicpg", REQUIRED_PARAMETER, FALSE, doc_ansicpg },
	{ "cell", SPECIAL_CHARACTER, FALSE, NULL, 0, "\t" }, /* Fake tables */
	{ "colortbl", DESTINATION, FALSE, NULL, 0, NULL, &colortbl_destination },
	{ "deff", REQUIRED_PARAMETER, FALSE, doc_deff },
	{ "field", DESTINATION, TRUE, NULL, 0, NULL, &field_destination },
	{ "fonttbl", DESTINATION, FALSE, NULL, 0, NULL, &fonttbl_destination },
	{ "footnote", DESTINATION, TRUE, doc_footnote, 0, NULL, &footnote_destination },
	{ "header", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination },
	{ "info", DESTINATION, FALSE, NULL, 0, NULL, &ignore_destination },
	{ "mac", NO_PARAMETER, FALSE, doc_mac },
	{ "pc", NO_PARAMETER, FALSE, doc_pc },
	{ "pca", NO_PARAMETER, FALSE, doc_pca },
	{ "row", SPECIAL_CHARACTER, FALSE, NULL, 0, "\n" }, /* Fake tables */
	{ "rtf", REQUIRED_PARAMETER, FALSE, doc_rtf },
	{ "stylesheet", DESTINATION, FALSE, NULL, 0, NULL, &stylesheet_destination },
	{ NULL }
};

const DestinationInfo document_destination = {
    document_word_table,
    document_text,
    (StateNewFunc *)attributes_new,
    (StateCopyFunc *)attributes_copy,
    (StateFreeFunc *)attributes_free
};

Attributes *
attributes_new(void)
{
    Attributes *attr = g_new0(Attributes, 1);
	attr->style = -1;

	attr->justification = -1;
	attr->pardirection = -1;
	attr->space_before = 0;
	attr->space_after = 0;
	attr->ignore_space_before = FALSE;
	attr->ignore_space_after = FALSE;
	attr->tabs = NULL;
	attr->left_margin = 0;
	attr->right_margin = 0;
	
	attr->background = -1;
	attr->foreground = -1;
	attr->font = -1;
	attr->size = -1;
	attr->italic = FALSE;
	attr->bold = FALSE;
	attr->smallcaps = FALSE;
	attr->strikethrough = FALSE;
	attr->subscript = FALSE;
	attr->superscript = FALSE;
	attr->underline = -1;
	attr->chardirection = -1;
	
	attr->unicode_skip = 1;
	
    return attr;
}

Attributes *
attributes_copy(Attributes *attr)
{
    Attributes *retval = g_new0(Attributes, 1);
	*retval = *attr;
	if(attr->tabs)
	    retval->tabs = pango_tab_array_copy(attr->tabs);
	
	return retval;
}

void
attributes_free(Attributes *attr)
{
    if(attr->tabs)
        pango_tab_array_free(attr->tabs);
	g_free(attr);
}

void
apply_attributes(ParserContext *ctx, Attributes *attr, GtkTextIter *start, GtkTextIter *end)
{
	if(attr->style != -1)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-style-%i", attr->style);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}
	
	if(attr->foreground != -1) 
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-foreground-%i", attr->foreground);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->background != -1) 
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-background-%i", attr->background);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->font != -1)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-font-%i", attr->font);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}
	else if(ctx->default_font != -1 && g_slist_length(ctx->font_table) > ctx->default_font)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-font-%i", ctx->default_font);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}
	
	if(attr->size != -1) 
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-fontsize-%i", attr->size);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->italic) 
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-italic", start, end);

	if(attr->bold)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-bold", start, end);

	if(attr->smallcaps)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-smallcaps", start, end);
	
	if(attr->strikethrough)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-strikethrough", start, end);

	if(attr->underline == PANGO_UNDERLINE_SINGLE)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-underline-single", start, end);

    if(attr->underline == PANGO_UNDERLINE_DOUBLE)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-underline-double", start, end);

	if(attr->underline == PANGO_UNDERLINE_ERROR)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-underline-wave", start, end);
	
	if(attr->justification == GTK_JUSTIFY_LEFT)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-left", start, end);

	if(attr->justification == GTK_JUSTIFY_RIGHT)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-right", start, end);

	if(attr->justification == GTK_JUSTIFY_CENTER)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-center", start, end);

	if(attr->justification == GTK_JUSTIFY_FILL)
        gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-justified", start, end);

	if(attr->pardirection == GTK_TEXT_DIR_RTL)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-right-to-left", start, end);
	
	if(attr->pardirection == GTK_TEXT_DIR_LTR)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-left-to-right", start, end);

	/* Character-formatting direction overrides paragraph formatting */
	if(attr->chardirection == GTK_TEXT_DIR_RTL)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-right-to-left", start, end);
	
	if(attr->chardirection == GTK_TEXT_DIR_LTR)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-left-to-right", start, end);
	
	if(attr->tabs != NULL)
	{
	    /* Create a separate tag for each PangoTabArray */
	    gchar *tagname = g_strdup_printf("libmac-rtf-tabs-%p", attr->tabs);
	    GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "tabs", attr->tabs, 
                     "tabs-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
	    g_free(tagname);
	    gtk_text_buffer_apply_tag(ctx->textbuffer, tag, start, end);
	}

	if(attr->space_before != 0 && !attr->ignore_space_before)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-space-before-%i", attr->space_before);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->space_after != 0 && !attr->ignore_space_after)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-space-after-%i", attr->space_after);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->subscript)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-subscript", start, end);
	
	if(attr->superscript)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-superscript", start, end);

	if(attr->left_margin != 0)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-left-margin-%i", attr->left_margin);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->right_margin != 0)
	{
		gchar *tagname = g_strdup_printf("libmac-rtf-right-margin-%i", attr->right_margin);
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, tagname, start, end);
		g_free(tagname);
	}

	if(attr->invisible)
		gtk_text_buffer_apply_tag_by_name(ctx->textbuffer, "libmac-rtf-invisible", start, end);
}

/*
 * Inserts the pending text with the current attributes. This function is 
 * called  whenever a change occurs in the attributes.
 */
void
document_text(ParserContext *ctx)
{
	GtkTextIter start, end;
	Attributes *attr; 
	int length;
	gchar *text;

	g_assert(ctx != NULL);
	
	attr = (Attributes *)get_state(ctx);
    text = ctx->text->str;
    
	if(text[0] == '\0') 
		return;

	length = strlen(text) - 1;
	if(!ctx->group_nesting_level && text[length] == '\n')
		text[length] = '\0';

	g_string_append(ctx->plaintext, text);

	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &end, ctx->endmark); /* shouldn't invalidate end, but it does? */
	gtk_text_buffer_insert(ctx->textbuffer, &end, text, -1);
	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &start, ctx->startmark);
	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &end, ctx->endmark);

	apply_attributes(ctx, attr, &start, &end);
	
	g_string_truncate(ctx->text, 0);
	
	/* Move the two marks back together again */
	gtk_text_buffer_move_mark(ctx->textbuffer, ctx->startmark, &end);
}

static gboolean
doc_ansi(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 1252;
    return TRUE;
}

static gboolean
doc_ansicpg(ParserContext *ctx, Attributes *attr, gint32 codepage, GError **error)
{
	ctx->codepage = codepage;
	return TRUE;
}

gboolean
doc_b(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-bold"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-bold");
        g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->bold = (param != 0);
    return TRUE;
}

gboolean
doc_cb(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	gchar *color;

	if((color = g_slist_nth_data(ctx->color_table, param)) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
		return FALSE;
	}
	
	gchar *tagname = g_strdup_printf("libmac-rtf-background-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "background", color, 
                     "background-set", TRUE, 
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
	g_free(tagname);
    
    attr->background = param;
    return TRUE;
}

gboolean
doc_cf(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	gchar *color;

	if((color = g_slist_nth_data(ctx->color_table, param)) == NULL)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_UNDEFINED_COLOR, _("Color '%i' undefined"), param);
		return FALSE;
	}
	
	gchar *tagname = g_strdup_printf("libmac-rtf-foreground-%i", param);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "foreground", color, 
                     "foreground-set", TRUE, 
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
	g_free(tagname);
    
    attr->foreground = param;
    return TRUE;
}

gboolean
doc_chftn(ParserContext *ctx, Attributes *attr, GError **error)
{
	gchar *footnotestring = g_strdup_printf("%d", ctx->footnote_number);
	g_string_append(ctx->text, footnotestring);
	g_free(footnotestring);
	return TRUE;
}

static gboolean
doc_deff(ParserContext *ctx, Attributes *attr, gint32 font, GError **error)
{
    ctx->default_font = font;
    return TRUE;
}

gboolean
doc_f(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	if(get_font_properties(ctx, param) == NULL)
	{
		g_warning(_("Font '%i' undefined"), param);
		return TRUE;
	}

    attr->font = param;
    return TRUE;
}

static gboolean
doc_footnote(ParserContext *ctx, Attributes *attr, GError **error)
{
	GtkTextIter iter;
	
	gtk_text_buffer_get_end_iter(ctx->textbuffer, &iter);
	gtk_text_buffer_insert(ctx->textbuffer, &iter, "\n", -1);
	/* Move the start and end marks back together */
	gtk_text_buffer_get_iter_at_mark(ctx->textbuffer, &iter, ctx->startmark);
	gtk_text_buffer_move_mark(ctx->textbuffer, ctx->endmark, &iter);
	return TRUE;
}

gboolean
doc_fs(ParserContext *ctx, Attributes *attr, gint32 halfpoints, GError **error)
{
    gchar *tagname = g_strdup_printf("libmac-rtf-fontsize-%i", halfpoints);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "size", HALF_POINTS_TO_PANGO(halfpoints), 
                     "size-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);
    
    attr->size = halfpoints;
    return TRUE;
}

gboolean
doc_i(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-italic"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-italic");
        g_object_set(tag, 
                     "style", PANGO_STYLE_ITALIC, 
                     "style-set", TRUE, 
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->italic = (param != 0);
    return TRUE;
}

gboolean
doc_li(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gchar *tagname = g_strdup_printf("libmac-rtf-left-margin-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "left-margin", PANGO_PIXELS(TWIPS_TO_PANGO(twips)), 
                     "left-margin-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);
    
    attr->left_margin = twips;
    return TRUE;
}

gboolean
doc_ltrch(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-left-to-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-left-to-right");
        g_object_set(tag, "direction", GTK_TEXT_DIR_LTR, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->chardirection = GTK_TEXT_DIR_LTR;
    return TRUE;
}

gboolean
doc_ltrpar(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-left-to-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-left-to-right");
        g_object_set(tag, "direction", GTK_TEXT_DIR_LTR, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->pardirection = GTK_TEXT_DIR_LTR;
    return TRUE;
}

static gboolean
doc_mac(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 10000;
    return TRUE;
}

gboolean
doc_pard(ParserContext *ctx, Attributes *attr, GError **error)
{
	attr->style = -1;
	attr->justification = -1;
    attr->pardirection = -1;
	attr->space_before = 0;
	attr->space_after = 0;
	attr->ignore_space_before = FALSE;
	attr->ignore_space_after = FALSE;
	attr->left_margin = 0;
	attr->right_margin = 0;
	if(attr->tabs)
	    pango_tab_array_free(attr->tabs);
	attr->tabs = NULL;
    return TRUE;
}

static gboolean
doc_pc(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 437;
    return TRUE;
}

static gboolean
doc_pca(ParserContext *ctx, Attributes *attr, GError **error)
{
    ctx->default_codepage = 850;
    return TRUE;
}

gboolean
doc_plain(ParserContext *ctx, Attributes *attr, GError **error)
{
	attr->style = -1;
	attr->background = -1;
	attr->foreground = -1;
	attr->font = -1;
	attr->size = -1;
	attr->italic = FALSE;
	attr->bold = FALSE;
	attr->smallcaps = FALSE;
	attr->strikethrough = FALSE;
	attr->underline = PANGO_UNDERLINE_NONE;
	attr->chardirection = -1;
	return TRUE;
}

gboolean
doc_qc(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-center"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-center");
        g_object_set(tag, 
                     "justification", GTK_JUSTIFY_CENTER, 
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_CENTER;
    return TRUE;
}

gboolean
doc_qj(ParserContext *ctx, Attributes *attr, GError **error)
{
	if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-justified"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-justified");
        g_object_set(tag, 
                     "justification", GTK_JUSTIFY_FILL, 
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
	attr->justification = GTK_JUSTIFY_FILL;
    return TRUE;
}

gboolean
doc_ql(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-left");
        g_object_set(tag, 
                     "justification", GTK_JUSTIFY_LEFT, 
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_LEFT;
    return TRUE;
}

gboolean
doc_qr(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-right"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-right");
        g_object_set(tag, 
                     "justification", GTK_JUSTIFY_RIGHT, 
                     "justification-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->justification = GTK_JUSTIFY_RIGHT;
    return TRUE;
}

gboolean
doc_ri(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gchar *tagname = g_strdup_printf("libmac-rtf-right-margin-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "right-margin", PANGO_PIXELS(TWIPS_TO_PANGO(twips)), 
                     "right-margin-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);
    
    attr->right_margin = twips;
    return TRUE;
}

static gboolean
doc_rtf(ParserContext *ctx, Attributes *attr, gint32 version, GError **error)
{
	if(version != 1)
	{
		g_set_error(error, RTF_ERROR, RTF_ERROR_BAD_VERSION, _("Unsupported RTF version '%i'"), (gint)version);
		return FALSE;
	}
	return TRUE;
}

gboolean
doc_rtlch(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-right-to-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-right-to-left");
        g_object_set(tag, "direction", GTK_TEXT_DIR_RTL, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->chardirection = GTK_TEXT_DIR_RTL;
    return TRUE;
}

gboolean
doc_rtlpar(ParserContext *ctx, Attributes *attr, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-right-to-left"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-right-to-left");
        g_object_set(tag, "direction", GTK_TEXT_DIR_RTL, NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->pardirection = GTK_TEXT_DIR_RTL;
    return TRUE;
}

gboolean
doc_s(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	gchar *tagname = g_strdup_printf("libmac-rtf-style-%i", param);
	if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
	{
		g_warning(_("Style '%i' undefined"), param);
		g_free(tagname);
		return TRUE;
	}
	g_free(tagname);
	attr->style = param;
	return TRUE;
}

gboolean
doc_sa(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gchar *tagname = g_strdup_printf("libmac-rtf-space-after-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "pixels-below-lines", PANGO_PIXELS(TWIPS_TO_PANGO(twips)), 
                     "pixels-below-lines-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);
    
    attr->space_after = twips;
    return TRUE;
}

gboolean
doc_saauto(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	attr->ignore_space_after = (param != 0);
	return TRUE;
}

gboolean
doc_sb(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gchar *tagname = g_strdup_printf("libmac-rtf-space-before-%i", twips);
    if(!gtk_text_tag_table_lookup(ctx->tags, tagname))
    {
        GtkTextTag *tag = gtk_text_tag_new(tagname);
        g_object_set(tag, 
                     "pixels-above-lines", PANGO_PIXELS(TWIPS_TO_PANGO(twips)), 
                     "pixels-above-lines-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    g_free(tagname);
    
    attr->space_before = twips;
    return TRUE;
}

gboolean
doc_sbauto(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	attr->ignore_space_before = (param != 0);
	return TRUE;
}

gboolean
doc_scaps(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-smallcaps"))
	{
		GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-smallcaps");
		g_object_set(tag,
		             "variant", PANGO_VARIANT_SMALL_CAPS,
		             "variant-set", TRUE,
		             NULL);
		gtk_text_tag_table_add(ctx->tags, tag);
	}
	attr->smallcaps = (param != 0);
	return TRUE;
}

gboolean
doc_strike(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-strikethrough"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-strikethrough");
        g_object_set(tag, 
                     "strikethrough", TRUE, 
                     "strikethrough-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->strikethrough = (param != 0);
    return TRUE;
}

gboolean
doc_sub(ParserContext *ctx, Attributes *attr, GError **error)
{
	if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-subscript"))
	{
		GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-subscript");
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(-6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
		gtk_text_tag_table_add(ctx->tags, tag);
	}
	attr->subscript = TRUE;
	return TRUE;
}

gboolean
doc_super(ParserContext *ctx, Attributes *attr, GError **error)
{
	if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-superscript"))
	{
		GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-superscript");
		g_object_set(tag,
		             "rise", POINTS_TO_PANGO(6),
		             "rise-set", TRUE,
		             "scale", PANGO_SCALE_X_SMALL,
		             "scale-set", TRUE,
		             NULL);
		gtk_text_tag_table_add(ctx->tags, tag);
	}
	attr->superscript = TRUE;
	return TRUE;
}

gboolean
doc_tx(ParserContext *ctx, Attributes *attr, gint32 twips, GError **error)
{
    gint tab_index;
    
    if(attr->tabs == NULL)
    {
        attr->tabs = pango_tab_array_new(1, FALSE);
        tab_index = 0;
    }
    else
    {
        tab_index = pango_tab_array_get_size(attr->tabs);
        pango_tab_array_resize(attr->tabs, tab_index + 1);
    }
    
    pango_tab_array_set_tab(attr->tabs, tab_index, PANGO_TAB_LEFT, TWIPS_TO_PANGO(twips));
        
    return TRUE;
}

gboolean
doc_u(ParserContext *ctx, Attributes *attr, gint32 ch, GError **error)
{
	gchar utf8[7];
	gint length, foo;
	gunichar code;

	if(ch < 0)
		code = ch + 65536;
	else
		code = ch;
	
	length = g_unichar_to_utf8(code, utf8);
	utf8[length] = '\0';
	g_string_append(ctx->text, utf8);
	
	for(foo = 0; foo < attr->unicode_skip; foo++)
		if(!skip_character_or_control_word(ctx, error))
			return FALSE;
	return TRUE;
}

gboolean
doc_uc(ParserContext *ctx, Attributes *attr, gint32 skip, GError **error)
{
	attr->unicode_skip = skip;
	return TRUE;
}

gboolean
doc_ul(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-underline-single"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-underline-single");
        g_object_set(tag, 
                     "underline", PANGO_UNDERLINE_SINGLE, 
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_uldb(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-underline-double"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-underline-double");
        g_object_set(tag, 
                     "underline", PANGO_UNDERLINE_DOUBLE, 
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_DOUBLE : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_ulnone(ParserContext *ctx, Attributes *attr, GError **error)
{
    attr->underline = PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_ulwave(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
    if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-underline-wave"))
    {
        GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-underline-wave");
        g_object_set(tag, 
                     "underline", PANGO_UNDERLINE_ERROR, 
                     "underline-set", TRUE,
                     NULL);
        gtk_text_tag_table_add(ctx->tags, tag);
    }
    attr->underline = param? PANGO_UNDERLINE_ERROR : PANGO_UNDERLINE_NONE;
    return TRUE;
}

gboolean
doc_v(ParserContext *ctx, Attributes *attr, gint32 param, GError **error)
{
	if(!gtk_text_tag_table_lookup(ctx->tags, "libmac-rtf-invisible"))
	{
		GtkTextTag *tag = gtk_text_tag_new("libmac-rtf-invisible");
		g_object_set(tag,
		             "invisible", TRUE,
		             "invisible-set", TRUE,
		             NULL);
		gtk_text_tag_table_add(ctx->tags, tag);
	}
	attr->invisible = (param != 0);
	return TRUE;
}
