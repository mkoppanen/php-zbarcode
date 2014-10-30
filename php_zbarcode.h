/*
   +----------------------------------------------------------------------+
   | PHP Version 5 / zbarcode                                             |
   +----------------------------------------------------------------------+
   | Copyright (c) 2009 Mikko Koppanen                                    |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Mikko Kopppanen <mkoppanen@php.net>                          |
   +----------------------------------------------------------------------+
*/
#ifndef _PHP_ZBARCODE_H_
# define _PHP_ZBARCODE_H_

#define PHP_ZBARCODE_EXTNAME "zbarcode"
#define PHP_ZBARCODE_EXTVER  "0.0.1-dev"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef ZTS
# include "TSRM.h"
#endif

/* Include magic wand header */
#if defined (IM_MAGICKWAND_HEADER_STYLE_SEVEN)
#  include <MagickWand/MagickWand.h>
#elif defined (IM_MAGICKWAND_HEADER_STYLE_OLD)
#  include <wand/magick-wand.h>
#else
#  include <wand/MagickWand.h>
#endif

#include "php.h"
#include "Zend/zend_exceptions.h"

#include <zbar.h>

/* {{{ typedef struct _php_zbarcode_object 
*/
typedef struct _php_zbarcode_object  {
	zend_object zo;
} php_zbarcode_object;
/* }}} */

/* {{{ typedef struct _php_zbarcode_scanner_object 
*/
typedef struct _php_zbarcode_scanner_object  {
	zend_object zo;
	zbar_image_scanner_t *scanner;
} php_zbarcode_scanner_object;
/* }}} */

/* {{{ typedef struct _php_zbarcode_image_object 
*/
typedef struct _php_zbarcode_image_object  {
	zend_object zo;
	MagickWand *magick_wand;
} php_zbarcode_image_object;
/* }}} */

#endif
