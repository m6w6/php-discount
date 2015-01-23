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

#ifndef PHP_DISCOUNT_H
#define PHP_DISCOUNT_H

extern zend_module_entry discount_module_entry;
#define phpext_discount_ptr &discount_module_entry

#define PHP_DISCOUNT_VERSION "1.1.0-dev"

#ifdef PHP_WIN32
#define PHP_DISCOUNT_API __declspec(dllexport)
#else
#define PHP_DISCOUNT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

static inline void *PHP_DISCOUNT_OBJ(zend_object *zo, zval *zv)
{
	if (!zo) {
		zo = Z_OBJ_P(zv);
	}
	return (char *) zo - zo->handlers->offset;
}

static inline zend_string *php_discount_cs2zs(char *s, size_t l)
{
	zend_string *str = erealloc(s, sizeof(*str) + l);

	memmove(str->val, str, l);
	str->val[l] = 0;
	str->len = l;
	str->h = 0;

	GC_REFCOUNT(str) = 1;
	GC_TYPE_INFO(str) = IS_STRING;

	return str;
}

#ifdef DISCOUNT_GLOBALS
ZEND_BEGIN_MODULE_GLOBALS(discount)
	void *dummy;
ZEND_END_MODULE_GLOBALS(discount)

ZEND_EXTERN_MODULE_GLOBALS(discount);

#ifdef ZTS
# define DISCOUNT_G(v) TSRMG(discount_globals_id, zend_discount_globals *, v)
#else
# define DISCOUNT_G(v) (discount_globals.v)
#endif
#endif

/* discount.c */
PHP_MINIT_FUNCTION(discount);
PHP_MSHUTDOWN_FUNCTION(discount);
PHP_RINIT_FUNCTION(discount);
PHP_RSHUTDOWN_FUNCTION(discount);
PHP_MINFO_FUNCTION(discount);

#endif	/* PHP_DISCOUNT_H */
