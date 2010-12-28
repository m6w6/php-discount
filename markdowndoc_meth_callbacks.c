/* TODO: copyright header */

#include <php.h>
#include <zend_exceptions.h>
#include <ext/spl/spl_exceptions.h>

#include "lib/mkdio.h"

#include "markdowndoc_class.h"
#include "markdowndoc_meth_callbacks.h"

static char *proxy_callback(
	const char			  *url,
	const int			  url_len,
	zend_fcall_info		  *fci,
	zend_fcall_info_cache *fcc,
	char				  *callback_name
	)
{
	zval			*zurl,
					*retval_ptr	= NULL,
					**params;
	int				retval;
	char			*result = NULL;
	TSRMLS_FETCH();

	if (fci == NULL || fci->size == 0)
		return NULL; /* should not happen */

	MAKE_STD_ZVAL(zurl);
	ZVAL_STRINGL(zurl, url, url_len, 1);
	params				= &zurl;
	fci->retval_ptr_ptr	= &retval_ptr;
	fci->params			= &params;
	fci->param_count	= 1;
	fci->no_separation	= 1;

	retval = zend_call_function(fci, fcc TSRMLS_CC);
	if (retval != SUCCESS ||fci->retval_ptr_ptr == NULL) {
		/* failure was most likely due to a previous exception (probably
			* in a previous URL), so don't throw yet another exception on
			* top of it */
		if (!EG(exception)) {
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,
				"Call to PHP %s callback has failed", callback_name);
		}
	} else {
		/* success in zend_call_function, but there may've been an exception */
		/* may have been changed by return by reference */
		retval_ptr = *fci->retval_ptr_ptr;
		if (retval_ptr == NULL) {
			/* no retval most likely an exception, but we feell confortable
			 * stacking an exception here */
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,
				"Call to PHP %s callback has failed (no return value)",
				callback_name);
		} else if (Z_TYPE_P(retval_ptr) == IS_NULL) {
			/* use the default string for the url */
		} else {
			if (Z_TYPE_P(retval_ptr) != IS_STRING) {
				SEPARATE_ZVAL(&retval_ptr);
				convert_to_string(retval_ptr);
			}
			result = estrndup(Z_STRVAL_P(retval_ptr), Z_STRLEN_P(retval_ptr));
		}
	}

	Z_DELREF_P(zurl);
	if (retval_ptr != NULL) {
		Z_DELREF_P(retval_ptr);
	}
	return result;
}

/* {{{ proxy_url_callback */
static char *proxy_url_callback(const char *url, const int url_len, void *data)
{
	discount_object	*dobj = data;
	return proxy_callback(url, url_len, dobj->url_fci, dobj->url_fcc, "URL");
}
/* }}} */

/* {{{ proxy_attributes_callback */
static char *proxy_attributes_callback(const char *url, const int url_len, void *data)
{
	discount_object	*dobj = data;
	return proxy_callback(url, url_len, dobj->attr_fci, dobj->attr_fcc,
		"attributes");
}
/* }}} */

/* {{{ proto bool MarkdownDocument::setUrlCallback(callback $url_callback) */
PHP_METHOD(markdowndoc, setUrlCallback)
{
	zend_fcall_info			fci;
	zend_fcall_info_cache	fcc;
	discount_object			*dobj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!", &fci, &fcc) == FAILURE) {
		RETURN_FALSE;
	}
	if ((dobj = markdowndoc_get_object(getThis(), 0 TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	if (fci.size > 0) { /* non-NULL passed */
		markdowndoc_store_callback(&fci, &fcc, &dobj->url_fci, &dobj->url_fcc);
		mkd_e_url(dobj->markdoc, proxy_url_callback);
		mkd_e_data(dobj->markdoc, dobj);
	} else { /* NULL */
		markdowndoc_free_callback(&dobj->url_fci, &dobj->url_fcc);
		mkd_e_url(dobj->markdoc, NULL);
	}
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool MarkdownDocument::setAttributesCallback(callback $attributes_callback) */
PHP_METHOD(markdowndoc, setAttributesCallback)
{
	zend_fcall_info			fci;
	zend_fcall_info_cache	fcc;
	discount_object			*dobj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!", &fci, &fcc) == FAILURE) {
		RETURN_FALSE;
	}
	if ((dobj = markdowndoc_get_object(getThis(), 0 TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	if (fci.size > 0) { /* non-NULL passed */
		markdowndoc_store_callback(&fci, &fcc, &dobj->attr_fci, &dobj->attr_fcc);
		mkd_e_flags(dobj->markdoc, proxy_url_callback);
		mkd_e_data(dobj->markdoc, dobj);
	} else { /* NULL */
		markdowndoc_free_callback(&dobj->attr_fci, &dobj->attr_fcc);
		mkd_e_url(dobj->markdoc, NULL);
	}
	
	RETURN_TRUE;
}
/* }}} */