/*
* Copyright (c) 2012, Gustavo Lopes
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*    * The names of its contributors may not be used to endorse or promote
*      products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* $Id$ */

#include <php.h>
#include <zend_exceptions.h>
#include <main/php_streams.h>
#include <ext/spl/spl_exceptions.h>

#include "lib/mkdio.h"

#include "php_discount.h"
#include "markdowndoc_class.h"
#include "markdowndoc_meth_input.h"

/* {{{ markdown_check_input_flags */
static int markdown_check_input_flags(mkd_flag_t flags)
{
	if (flags & ~(MKD_TABSTOP|MKD_NOHEADER)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"Only the flags TABSTOP and NOHEADER are allowed.");
		return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ markdown_init_from_stream */
static int markdown_init_from_stream(zval* obj, zval *zstream, long flags)
{
	discount_object *dobj = PHP_DISCOUNT_OBJ(NULL, obj);
	MMIOT			*mmiot;
	php_stream		*stream;
	int				close;
	FILE			*f;
	int				ret;

	if (dobj->markdoc != NULL) {
		zend_throw_exception_ex(spl_ce_LogicException, 0,
			"This object has already been initialized.");
		return FAILURE;
	}

	if (markdown_check_input_flags((mkd_flag_t) flags) == FAILURE) {
		return FAILURE;
	}

	if (markdowndoc_get_file(zstream, 0, &stream, &close, &f) == FAILURE) {
		return FAILURE;
	}

	mmiot = mkd_in(f, (mkd_flag_t) flags);
	if (mmiot == NULL) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"Error initializing markdown document: call to the library routine "
			"mkd_in() failed");
		ret = FAILURE;
	} else {
		dobj->markdoc = mmiot;
		ret = SUCCESS;
	}

	if (close) {
		php_stream_close(stream);
	}
	return ret;
}
/* }}} */

/* {{{ markdown_init_from_string */
static int markdown_init_from_string(zval* obj, const char *string, int len, long flags)
{
	discount_object *dobj = PHP_DISCOUNT_OBJ(NULL, obj);
	MMIOT			*mmiot;

	if (dobj->markdoc != NULL) {
		zend_throw_exception_ex(spl_ce_LogicException, 0,
			"This object has already been initialized.");
		return FAILURE;
	}

	if (markdown_check_input_flags((mkd_flag_t) flags) == FAILURE) {
		return FAILURE;
	}

	mmiot = mkd_string((char*) string, len, (mkd_flag_t) flags);
	if (mmiot == NULL) {
		/* should not happen */
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"Error initializing markdown document: call to the library routine "
			"mkd_string() failed");
		return FAILURE;
	}

	dobj->markdoc = mmiot;
	return SUCCESS;
}
/* }}} */

/* {{{ proto MarkdownDoc MarkdownDocument::createFromStream(mixed $markdown_stream [, int $flags = 0])
 *	   Creates and initializes a markdown document from a stream. */
PHP_METHOD(markdowndoc, createFromStream)
{
	zval		*zstream;
	zend_long	flags	= 0;
	int			ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &zstream, &flags) == FAILURE) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, markdowndoc_ce);
	ret = markdown_init_from_stream(return_value, zstream, flags);
	if (ret == FAILURE) {
		zval_dtor(return_value);
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto MarkdownDoc MarkdownDocument::createFromString(string $markdown_string [, int $flags = 0])
 *	   Creates and initializes a markdown document from a string. */
PHP_METHOD(markdowndoc, createFromString)
{
	char		*string;
	size_t		string_len;
	zend_long	flags	= 0;
	int			ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l",
			&string, &string_len, &flags) == FAILURE) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, markdowndoc_ce);
	ret = markdown_init_from_string(return_value, string, string_len, flags);
	if (ret == FAILURE) {
		zval_dtor(return_value);
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool MarkdownDocument::initFromStream(mixed $markdown_stream [, int $flags = 0])
 *	   Initializes a markdown document from a stream. */
PHP_METHOD(markdowndoc, initFromStream)
{
	zval			*instance, *zstream;
	zend_long		flags	= 0;
	int				ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oz|l",
			&instance, markdowndoc_ce, &zstream, &flags) == FAILURE) {
		RETURN_FALSE;
	}

	ret = markdown_init_from_stream(instance, zstream, flags);
	if (ret == FAILURE) {
		RETURN_FALSE; /* no rollback needed */
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool MarkdownDocument::initFromString(string $markdown_string [, int $flags = 0])
 *	   Initializes a markdown document from a string. */
PHP_METHOD(markdowndoc, initFromString)
{
	zval			*instance;
	char			*string;
	size_t			string_len;
	zend_long		flags	= 0;
	int				ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Os|l",
			&instance, markdowndoc_ce, &string, &string_len, &flags) == FAILURE) {
		RETURN_FALSE;
	}

	ret = markdown_init_from_string(instance, string, string_len, flags);
	if (ret == FAILURE) {
		RETURN_FALSE; /* no rollback needed */
	}
	
	RETURN_TRUE;
}
/* }}} */
