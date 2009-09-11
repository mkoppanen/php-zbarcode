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

#include "php_zbarcode.h"

/* Handlers */
static zend_object_handlers php_zbarcode_object_handlers;
static zend_object_handlers php_zbarcode_scanner_object_handlers;
static zend_object_handlers php_zbarcode_image_object_handlers;

/* Class entries */
zend_class_entry *php_zbarcode_sc_entry;
zend_class_entry *php_zbarcode_image_sc_entry;
zend_class_entry *php_zbarcode_scanner_sc_entry;
zend_class_entry *php_zbarcode_exception_class_entry;

#define PHP_ZBARCODE_THROW_IMAGE_EXCEPTION(magick_wand, alternative_message) \
{ \
	ExceptionType severity; \
	char *message = MagickGetException(magick_wand, &severity); \
	MagickClearException(magick_wand); \
	if (message != NULL && strlen(message)) { \
		zend_throw_exception(php_zbarcode_exception_class_entry, message, 1 TSRMLS_CC); \
		message = (char *)MagickRelinquishMemory(message); \
		message = NULL; \
		return; \
	} else { \
		if (message) \
			message = (char *)MagickRelinquishMemory(message); /* Free possible empty message */ \
		zend_throw_exception(php_zbarcode_exception_class_entry, alternative_message, 1 TSRMLS_CC); \
		return; \
	} \
}

#define PHP_ZBARCODE_CHAIN_METHOD RETURN_ZVAL(getThis(), 1, 0);

static zend_bool _php_zbarcode_read(MagickWand *wand, char *filename, zend_bool enhance)
{
	if (enhance) {
		MagickSetResolution(wand, 200, 200);
	} 
	
	if (MagickReadImage(wand, filename) == MagickFalse) {
		ClearMagickWand(wand);
		return 0;
	}
	
	if (enhance) {
		MagickEnhanceImage(wand);
	}
	return 1;
}

/* {{{ zBarcodeImage zBarcodeImage::__construct([string filename])
	Construct a new zBarcodeImage object
*/
PHP_METHOD(zbarcodeimage, __construct)
{
	php_zbarcode_image_object *intern;
	char *filename = NULL;
	int filename_len = 0;
	zend_bool enhance = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!b", &filename, &filename_len, &enhance) == FAILURE) {
		return;
	}
	
	if (!filename) {
		return;
	}

	intern = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!_php_zbarcode_read(intern->magick_wand, filename, enhance)) {
		PHP_ZBARCODE_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}
	return;
}
/* }}} */

/* {{{ zBarcodeImage zBarcodeImage::read(string filename[, boolean enhance])
	Read an image
*/
PHP_METHOD(zbarcodeimage, read)
{
	php_zbarcode_image_object *intern;
	char *filename;
	int filename_len;
	zend_bool enhance = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &filename, &filename_len, &enhance) == FAILURE) {
		return;
	}

	intern = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!_php_zbarcode_read(intern->magick_wand, filename, enhance)) {
		PHP_ZBARCODE_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}
	PHP_ZBARCODE_CHAIN_METHOD;
}
/* }}} */

/* {{{ zBarcodeImage zBarcodeImage:clear()
	Remove existing pages
*/
PHP_METHOD(zbarcodeimage, clear)
{
	php_zbarcode_image_object *intern;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	ClearMagickWand(intern->magick_wand);
	
	PHP_ZBARCODE_CHAIN_METHOD;
}
/* }}} */

/* {{{ integer zBarcodeImage:count()
	Count pages
*/
PHP_METHOD(zbarcodeimage, count)
{
	php_zbarcode_image_object *intern;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(MagickGetNumberImages(intern->magick_wand));
}
/* }}} */

static zbar_image_t *_php_zbarcode_get_page(MagickWand *wand) 
{
	unsigned long width, height;
	unsigned char *image_data;
	zbar_image_t *image;
	
	size_t image_size;
	
	if (MagickSetImageDepth(wand, 8) == MagickFalse) {
		return NULL;
	}

	if (MagickSetImageFormat(wand, "GRAY") == MagickFalse) {
		return NULL;
	}
	
	width  = MagickGetImageWidth(wand);
	height = MagickGetImageHeight(wand);

    image_data = malloc(width * height);
	
	if (!MagickExportImagePixels(wand, 0, 0, width, height, "I", CharPixel, image_data)) {
		return NULL;
	}
	
	image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, (void *)image_data, image_size, zbar_image_free_data);
    
	return image;
}

static zval *_php_zbarcode_scan_page(zbar_image_scanner_t *scanner, zbar_image_t *image, zend_bool extended TSRMLS_DC)
{
	int n;
	const zbar_symbol_t *symbol;
	zval *return_array;
	
	/* Return an array */
	MAKE_STD_ZVAL(return_array);
	array_init(return_array);
		
	/* scan the image for barcodes */
    n = zbar_scan_image(scanner, image);

	/* extract results */
	symbol = zbar_image_first_symbol(image);
	
	/* Loop through all all symbols */
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
		zval *symbol_array, *loc_array;
		zbar_symbol_type_t symbol_type;
		const char *data;
		unsigned int loc_size;

		/* Initialize array element */
		MAKE_STD_ZVAL(symbol_array);
		array_init(symbol_array);

		/* Get symbol type and data in it */
		symbol_type = zbar_symbol_get_type(symbol);
        data = zbar_symbol_get_data(symbol);

		add_assoc_string(symbol_array, "data", (char *)data, 1);
		add_assoc_string(symbol_array, "type", (char *)zbar_get_symbol_name(symbol_type), 1);
#ifdef HAVE_ZBAR_09
    	add_assoc_long(symbol_array, "quality", zbar_symbol_get_quality(symbol));
#endif		
		if (extended) {
			unsigned int i;
			
			MAKE_STD_ZVAL(loc_array);
			array_init(loc_array);
			loc_size = zbar_symbol_get_loc_size(symbol);

			for (i = 0; i < loc_size; i++) {
				zval *coords;
				MAKE_STD_ZVAL(coords);
				array_init(coords);

				add_assoc_long(coords, "x", zbar_symbol_get_loc_x(symbol, i));
				add_assoc_long(coords, "y", zbar_symbol_get_loc_y(symbol, i));

				add_next_index_zval(loc_array, coords);	
			}
			add_assoc_zval(symbol_array, "location", loc_array);
		}
		add_next_index_zval(return_array, symbol_array);
	}
	return return_array;
}

/* {{{ array zBarCodeScanner::scan(zBarCodeImage image[, int page_num, bool extended])
	Scan an image
*/
PHP_METHOD(zbarcodescanner, scan)
{
	zbar_image_t *page;
	php_zbarcode_scanner_object *intern;
	php_zbarcode_image_object   *intern_image;
	zval *object;
	int i = 1;
	zend_bool extended = 0;
	long page_num = 1, image_count;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|lb", &object, php_zbarcode_image_sc_entry, &page_num, &extended) == FAILURE) {
		return;
	}
	
	intern       = (php_zbarcode_scanner_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern_image = (php_zbarcode_image_object *)zend_object_store_get_object(object TSRMLS_CC);
	
	image_count = MagickGetNumberImages(intern_image->magick_wand);
	
	if (image_count == 0) {
		zend_throw_exception(php_zbarcode_exception_class_entry, "The image object does not contain images", 1 TSRMLS_CC);
		return;
	}
	
	array_init(return_value);
	
	if (page_num > 0) {
		if (image_count < page_num) {
			zend_throw_exception_ex(php_zbarcode_exception_class_entry, 1 TSRMLS_CC, "Invalid page specified. The object contains %d page(s)", image_count);
			return;
		}

		if (MagickSetIteratorIndex(intern_image->magick_wand, page_num - 1) == MagickFalse) {
			zend_throw_exception(php_zbarcode_exception_class_entry, "Failed to set the page number", 1 TSRMLS_CC);
			return;
		} 
		/* Read page */
		page = _php_zbarcode_get_page(intern_image->magick_wand);
		
		if (!page) {
			zend_throw_exception(php_zbarcode_exception_class_entry, "Failed to get the page", 1 TSRMLS_CC);
			return;
		}

		/* Scan the page for barcodes */
		*return_value = *_php_zbarcode_scan_page(intern->scanner, page, extended TSRMLS_CC);
		
	} else {
		MagickResetIterator(intern_image->magick_wand);
		while (MagickNextImage(intern_image->magick_wand) != MagickFalse) {
			zval *page_array;

			/* Read the current page */
			page = _php_zbarcode_get_page(intern_image->magick_wand);

			/* Reading current page failed */
			if (!page) {
				i++;
				continue;
			}
			/* Scan the page for barcodes */
			page_array = _php_zbarcode_scan_page(intern->scanner, page, extended TSRMLS_CC);
			add_index_zval(return_value, i++, page_array);
		}
	}
	return;
}
/* }}} */

/* {{{ zBarCodeScanner zBarCodeScanner::setConfig(int name, int value[, int symbology])
	Set config option
*/
PHP_METHOD(zbarcodescanner, setconfig)
{
	php_zbarcode_scanner_object *intern;
	long symbology = 0, name, value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|l", &name, &value, &symbology) == FAILURE) {
		return;
	}
	intern = (php_zbarcode_scanner_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zbar_image_scanner_set_config(intern->scanner, symbology, name, value) != 0) {
		zend_throw_exception(php_zbarcode_exception_class_entry, "Config does not apply to specified symbology, or value out of range", 1 TSRMLS_CC);
		return;
	}
	PHP_ZBARCODE_CHAIN_METHOD;
}
/* }}} */

static function_entry php_zbarcode_class_methods[] =
{
	{ NULL, NULL, NULL }
};

ZEND_BEGIN_ARG_INFO_EX(zbarcode_image_construct_args, 0, 0, 0)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, enhance)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(zbarcode_image_read_args, 0, 0, 1)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, enhance)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(zbarcode_image_no_args, 0, 0, 0)
ZEND_END_ARG_INFO()

static function_entry php_zbarcode_image_class_methods[] =
{
	PHP_ME(zbarcodeimage, __construct,	zbarcode_image_construct_args, 	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(zbarcodeimage, read,			zbarcode_image_read_args, 		ZEND_ACC_PUBLIC)
	PHP_ME(zbarcodeimage, clear,		zbarcode_image_no_args, 		ZEND_ACC_PUBLIC)
	PHP_ME(zbarcodeimage, count,		zbarcode_image_no_args, 		ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

ZEND_BEGIN_ARG_INFO_EX(zbarcode_scanner_scan_args, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, zBarCodeImage, zBarCodeImage, 0) 
	ZEND_ARG_INFO(0, page_num)
	ZEND_ARG_INFO(0, extended)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(zbarcode_scanner_setconfig_args, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, symbology)
ZEND_END_ARG_INFO()

static function_entry php_zbarcode_scanner_class_methods[] =
{
	PHP_ME(zbarcodescanner, scan,		zbarcode_scanner_scan_args, 		ZEND_ACC_PUBLIC)
	PHP_ME(zbarcodescanner, setconfig,	zbarcode_scanner_setconfig_args, 	ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

/* {{{ static void php_zbarcode_object_free_storage(void *object TSRMLS_DC)
*/
static void php_zbarcode_object_free_storage(void *object TSRMLS_DC)
{
	php_zbarcode_scanner_object *intern = (php_zbarcode_scanner_object *)object;
	
	if (!intern) {
		return;
	}
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}
/* }}} */

/* {{{ static void php_zbarcode_scanner_object_free_storage(void *object TSRMLS_DC)
*/
static void php_zbarcode_scanner_object_free_storage(void *object TSRMLS_DC)
{
	php_zbarcode_scanner_object *intern = (php_zbarcode_scanner_object *)object;
	
	if (!intern) {
		return;
	}
	
	if (intern->scanner) {
		zbar_image_scanner_destroy(intern->scanner);
		intern->scanner = NULL;
	}
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}
/* }}} */

/* {{{ static void php_zbarcode_image_object_free_storage(void *object TSRMLS_DC)
*/
static void php_zbarcode_image_object_free_storage(void *object TSRMLS_DC)
{
	php_zbarcode_image_object *intern = (php_zbarcode_image_object *)object;
	
	if (!intern) {
		return;
	}

	if (intern->magick_wand) {
		DestroyMagickWand(intern->magick_wand);
		intern->magick_wand = NULL;
	}
	
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}
/* }}} */

/* {{{ static zend_object_value php_zbarcode_object_new(zend_class_entry *class_type TSRMLS_DC)
*/
static zend_object_value php_zbarcode_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_zbarcode_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_zbarcode_scanner_object));
	memset(&intern->zo, 0, sizeof(php_zbarcode_scanner_object));
	
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_zbarcode_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_zbarcode_object_handlers;
	return retval;
}
/* }}} */

/* {{{ static zend_object_value php_zbarcode_scanner_object_new(zend_class_entry *class_type TSRMLS_DC)
*/
static zend_object_value php_zbarcode_scanner_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_zbarcode_scanner_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_zbarcode_scanner_object));
	memset(&intern->zo, 0, sizeof(php_zbarcode_scanner_object));
	
	/* Initialize reader */
	intern->scanner = zbar_image_scanner_create();
	
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_zbarcode_scanner_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_zbarcode_scanner_object_handlers;
	return retval;
}
/* }}} */

/* {{{ static zend_object_value php_zbarcode_image_object_new(zend_class_entry *class_type TSRMLS_DC)
*/
static zend_object_value php_zbarcode_image_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_zbarcode_image_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_zbarcode_image_object));
	memset(&intern->zo, 0, sizeof(php_zbarcode_image_object));
	
	/* Initialize reader */
	intern->magick_wand = NewMagickWand();
	
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
	
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_zbarcode_image_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_zbarcode_image_object_handlers;
	return retval;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(zbarcode)
*/
PHP_MINIT_FUNCTION(zbarcode)
{
	zend_class_entry ce;
	memcpy(&php_zbarcode_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&php_zbarcode_scanner_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&php_zbarcode_image_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	
	MagickWandGenesis();
	
	/*
		Initialize exceptions (zbarcode exception)
	*/
	INIT_CLASS_ENTRY(ce, "zbarcodeexception", NULL);
	php_zbarcode_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_zbarcode_exception_class_entry->ce_flags |= ZEND_ACC_FINAL;

	/*
		Initialize the class (zbarcode). This class is just a container for constants
	*/
	INIT_CLASS_ENTRY(ce, "zbarcode", php_zbarcode_class_methods);
	ce.create_object = php_zbarcode_object_new;
	php_zbarcode_object_handlers.clone_obj = NULL;
	php_zbarcode_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/*
		Initialize the class (zbarcode image)
	*/
	INIT_CLASS_ENTRY(ce, "zbarcodeimage", php_zbarcode_image_class_methods);
	ce.create_object = php_zbarcode_image_object_new;
	php_zbarcode_image_object_handlers.clone_obj = NULL;
	php_zbarcode_image_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/*
		Initialize the class (zbarcode scanner)
	*/
	INIT_CLASS_ENTRY(ce, "zbarcodescanner", php_zbarcode_scanner_class_methods);
	ce.create_object = php_zbarcode_scanner_object_new;
	php_zbarcode_scanner_object_handlers.clone_obj = NULL;
	php_zbarcode_scanner_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

#define PHP_ZBARCODE_REGISTER_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(php_zbarcode_sc_entry, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC);

	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_ENABLE", ZBAR_CFG_ENABLE);			/**< enable symbology/feature */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_ADD_CHECK", ZBAR_CFG_ADD_CHECK);		/**< enable check digit when optional */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_EMIT_CHECK", ZBAR_CFG_EMIT_CHECK);	/**< return check digit when present */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_ASCII", ZBAR_CFG_ASCII);				/**< enable full ASCII character set */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_NUM", ZBAR_CFG_NUM);					/**< number of boolean decoder configs */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_MIN_LEN", ZBAR_CFG_MIN_LEN);			/**< minimum data length for valid decode */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_MAX_LEN", ZBAR_CFG_MAX_LEN);			/**< maximum data length for valid decode */
#ifdef HAVE_ZBAR_09
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_POSITION", ZBAR_CFG_POSITION);		/**< enable scanner to collect position data */
#endif
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_X_DENSITY", ZBAR_CFG_X_DENSITY);		/**< image scanner vertical scan density */
	PHP_ZBARCODE_REGISTER_CONST_LONG("CFG_Y_DENSITY", ZBAR_CFG_Y_DENSITY);		/**< image scanner horizontal scan density */
    
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ALL", 0);					/**< all symbologies */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_NONE", ZBAR_NONE);			/**< no symbol decoded */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_PARTIAL", ZBAR_PARTIAL);		/**< intermediate status */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_EAN8", ZBAR_EAN8);			/**< EAN-8 */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_UPCE", ZBAR_UPCE);			/**< UPC-E */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ISBN10", ZBAR_ISBN10);		/**< ISBN-10 (from EAN-13).*/
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_UPCA", ZBAR_UPCA);			/**< UPC-A */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_EAN13", ZBAR_EAN13);			/**< EAN-13 */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ISBN13", ZBAR_ISBN13);		/**< ISBN-13 (from EAN-13).  */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_I25", ZBAR_I25);				/**< Interleaved 2 of 5. */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_CODE39", ZBAR_CODE39);		/**< Code 39.  */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_PDF417", ZBAR_PDF417);		/**< PDF417. */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_CODE128", ZBAR_CODE128);		/**< Code 128 */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_SYMBOL", ZBAR_SYMBOL);		/**< mask for base symbol type */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ADDON2", ZBAR_ADDON2);		/**< 2-digit add-on flag */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ADDON5", ZBAR_ADDON5);		/**< 5-digit add-on flag */
	PHP_ZBARCODE_REGISTER_CONST_LONG("SYM_ADDON", ZBAR_ADDON);			/**< add-on flag mask */
      
#undef PHP_ZBARCODE_REGISTER_CONST_LONG                                                        

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION(zbarcode)
*/
PHP_MSHUTDOWN_FUNCTION(zbarcode)
{
	MagickWandTerminus();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION(zbarcode)
*/
PHP_MINFO_FUNCTION(zbarcode)
{
	int major = 0, minor = 0;
	char *zbar_ver;
	unsigned long magick_version;

	zbar_version(&major, &minor);
	spprintf(&zbar_ver, 512, "%d.%d", major, minor);

	php_info_print_table_start();
	php_info_print_table_row(2, "zbarcode module", "enabled");
	php_info_print_table_row(2, "zbarcode module version", PHP_ZBARCODE_EXTVER);
	php_info_print_table_row(2, "ZBar library version", zbar_ver);
	php_info_print_table_row(2, "ImageMagick version", MagickGetVersion(&magick_version));
	php_info_print_table_end();
	
	efree(zbar_ver);
}
/* }}} */

static function_entry php_zbarcode_functions[] =
{
	{ NULL, NULL, NULL }
};

zend_module_entry zbarcode_module_entry =
{
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_ZBARCODE_EXTNAME,
	php_zbarcode_functions,		/* Functions */
	PHP_MINIT(zbarcode),		/* MINIT */
	PHP_MSHUTDOWN(zbarcode),	/* MSHUTDOWN */
	NULL,						/* RINIT */
	NULL,						/* RSHUTDOWN */
	PHP_MINFO(zbarcode),		/* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_ZBARCODE_EXTVER,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ZBARCODE
ZEND_GET_MODULE(zbarcode)
#endif