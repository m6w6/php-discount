/*
 * mkdio -- markdown front end input functions
 *
 * Copyright (C) 2007 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "cstring.h"
#include "markdown.h"
#include "amalloc.h"

/* on merge: moved config.h last to avoid conflict with windows headers */
#include "config.h"

typedef ANCHOR(Line) LineAnchor;

/* create a new blank Document
 */
static Document*
new_Document()
{
    Document *ret = ecalloc(sizeof(Document), 1);

    if ( ret ) {
	if (( ret->ctx = ecalloc(sizeof(MMIOT), 1) )) {
	    ret->magic = VALID_DOCUMENT;
	    return ret;
	}
	efree(ret);
    }
    return 0;
}


/* add a line to the markdown input chain
 */
static void
queue(Document* a, Cstring *line)
{
    Line *p = ecalloc(sizeof *p, 1);
    unsigned char c;
    int xp = 0;
    int           size = S(*line);
    unsigned char *str = (unsigned char*)T(*line);

    CREATE(p->text);
    ATTACH(a->content, p);

    while ( size-- ) {
	if ( (c = *str++) == '\t' ) {
	    /* expand tabs into ->tabstop spaces.  We use ->tabstop
	     * because the ENTIRE FREAKING COMPUTER WORLD uses editors
	     * that don't do ^T/^D, but instead use tabs for indentation,
	     * and, of course, set their tabs down to 4 spaces 
	     */
	    do {
		EXPAND(p->text) = ' ';
	    } while ( ++xp % a->tabstop );
	}
	else if ( c >= ' ' ) {
	    EXPAND(p->text) = c;
	    ++xp;
	}
    }
    EXPAND(p->text) = 0;
    S(p->text)--;
    p->dle = mkd_firstnonblank(p);
}


/* trim leading blanks from a header line
 */
static void
header_dle(Line *p)
{
    CLIP(p->text, 0, 1);
    p->dle = mkd_firstnonblank(p);
}


/* build a Document from any old input.
 */
typedef int (*getc_func)(void*);

Document *
populate(getc_func getc, void* ctx, int flags)
{
    Cstring line;
    Document *a = new_Document();
    int c;
    int pandoc = 0;

    if ( !a ) return 0;

    a->tabstop = (flags & MKD_TABSTOP) ? 4 : TABSTOP;

    CREATE(line);

    while ( (c = (*getc)(ctx)) != EOF ) {
	if ( c == '\n' ) {
	    if ( pandoc != EOF && pandoc < 3 ) {
		if ( S(line) && (T(line)[0] == '%') )
		    pandoc++;
		else
		    pandoc = EOF;
	    }
	    queue(a, &line);
	    S(line) = 0;
	}
	else if ( isprint(c) || isspace(c) || (c & 0x80) )
	    EXPAND(line) = c;
    }

    if ( S(line) )
	queue(a, &line);

    DELETE(line);

    if ( (pandoc == 3) && !(flags & (MKD_NOHEADER|MKD_STRICT)) ) {
	/* the first three lines started with %, so we have a header.
	 * clip the first three lines out of content and hang them
	 * off header.
	 */
	Line *headers = T(a->content);

	a->title = headers;             header_dle(a->title);
	a->author= headers->next;       header_dle(a->author);
	a->date  = headers->next->next; header_dle(a->date);

	T(a->content) = headers->next->next->next;
    }

    return a;
}


/* convert a file into a linked list
 */
Document *
mkd_in(FILE *f, DWORD flags)
{
    return populate((getc_func)fgetc, f, flags & INPUT_MASK);
}


/* return a single character out of a buffer
 */
struct string_ctx {
    char *data;		/* the unread data */
    int   size;		/* and how much is there? */
} ;


static int
strget(struct string_ctx *in)
{
    if ( !in->size ) return EOF;

    --(in->size);

    return *(in->data)++;
}


/* convert a block of text into a linked list
 */
Document *
mkd_string(char *buf, int len, DWORD flags)
{
    struct string_ctx about;

    about.data = buf;
    about.size = len;

    return populate((getc_func)strget, &about, flags & INPUT_MASK);
}


/* write the html to a file (xmlified if necessary)
 */
int
mkd_generatehtml(Document *p, FILE *output)
{
    char *doc;
    int szdoc;

	/* on merge: changed meaning of return value */

    if ( (szdoc = mkd_document(p, &doc)) != EOF ) {
		int ret = 0;

		if ( p->ctx->flags & MKD_CDATA )
			ret |= mkd_generatexml(doc, szdoc, output);
		else
			ret = !fwrite(doc, szdoc, 1, output);

		ret |= putc('\n', output);
		/* on merge: added call to efree */
		efree(doc);
		return ret;
    }
    return EOF;
}


/* convert some markdown text to html
 */
int
markdown(Document *document, FILE *out, int flags)
{
    if ( mkd_compile(document, flags) ) {
	mkd_generatehtml(document, out);
	mkd_cleanup(document);
	return 0;
    }
    return -1;
}


/* write out a Cstring, mangled into a form suitable for `<a href=` or `<a id=`
 */
void
mkd_string_to_anchor(char *s, int len, void(*outchar)(int,void*),
				       void *out, int labelformat)
{
    unsigned char c;

    int i, size;
    char *line;

    size = mkd_line(s, len, &line, IS_LABEL);
    
    if ( labelformat && size && !isalpha(line[0]) )
	(*outchar)('L',out);
    for ( i=0; i < size ; i++ ) {
	c = line[i];
	if ( labelformat ) {
	    if ( isalnum(c) || (c == '_') || (c == ':') || (c == '-') || (c == '.' ) )
		(*outchar)(c, out);
	    else
		(*outchar)('.',out);
	}
	else
	    (*outchar)(c,out);
    }
	
    if (line)
	efree(line);
}


/*  ___mkd_reparse() a line
 */
static void
mkd_parse_line(char *bfr, int size, MMIOT *f, int flags)
{
    ___mkd_initmmiot(f, 0);
    f->flags = flags & USER_FLAGS;
    ___mkd_reparse(bfr, size, 0, f);
    ___mkd_emblock(f);
}


/* ___mkd_reparse() a line, returning it in malloc()ed memory
 */
int
mkd_line(char *bfr, int size, char **res, DWORD flags)
{
    MMIOT f;
    int len;
    
    mkd_parse_line(bfr, size, &f, flags);

    if ( len = S(f.out) ) {
	/* kludge alert;  we know that T(f.out) is malloced memory,
	 * so we can just steal it away.   This is awful -- there
	 * should be an opaque method that transparently moves 
	 * the pointer out of the embedded Cstring.
	 */
	EXPAND(f.out) = 0;
	*res = T(f.out);
	T(f.out) = 0;
	S(f.out) = ALLOCATED(f.out) = 0;
    }
    else {
	 *res = 0;
	 len = EOF;
     }
    ___mkd_freemmiot(&f, 0);
    return len;
}


/* ___mkd_reparse() a line, writing it to a FILE
 */
int
mkd_generateline(char *bfr, int size, FILE *output, DWORD flags)
{
    MMIOT f;
	int ret;

	/* on merge: changed so the return value has some meaning */

    mkd_parse_line(bfr, size, &f, flags);
    if ( flags & MKD_CDATA ) {
		ret = mkd_generatexml(T(f.out), S(f.out), output);
	} else {
		if (S(f.out) == 0)
			ret = 0;
		else
			ret = fwrite(T(f.out), S(f.out), 1, output) == 1 ? 0 : EOF;
	}

    ___mkd_freemmiot(&f, 0);
    return ret;
}


/* set the url display callback
 */
void
mkd_e_url(Document *f, mkd_callback_t edit)
{
    if ( f )
	f->cb.e_url = edit;
}


/* set the url options callback
 */
void
mkd_e_flags(Document *f, mkd_callback_t edit)
{
    if ( f )
	f->cb.e_flags = edit;
}


/* set the url display/options deallocator
 */
void
mkd_e_free(Document *f, mkd_free_t dealloc)
{
    if ( f )
	f->cb.e_free = dealloc;
}


/* set the url display/options context data field
 */
void
mkd_e_data(Document *f, void *data)
{
    if ( f )
	f->cb.e_data = data;
}
