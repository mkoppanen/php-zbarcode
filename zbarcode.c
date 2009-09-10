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

static zbar_image_t *_php_zbarcode_read(MagickWand *wand, char *filename)
{
	zbar_image_t *image;
	unsigned int width, height;
	unsigned char *image_data;
	size_t image_size;
	
	if (MagickReadImage(wand, filename) == MagickFalse) {
		return NULL;
	}
	
	if (MagickSetImageDepth(wand, 8) == MagickFalse) {
		ClearMagickWand(wand);
		return NULL;
	}

	if (MagickSetImageFormat(wand, "GRAY") == MagickFalse) {
		ClearMagickWand(wand);
		return NULL;
	}
	
	image_data = MagickGetImageBlob(wand, &image_size);
	
	if (!image_data) {
		ClearMagickWand(wand);
		return NULL;
	}
	
	width  = MagickGetImageWidth(wand);
	height = MagickGetImageHeight(wand);
	
	image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, (void *)image_data, width * height, zbar_image_free_data);
    
	ClearMagickWand(wand);
	return image;
}

/* {{{ zBarcodeImage zBarcodeImage::__construct([string filename])
	Construct a new zBarcodeImage object
*/
PHP_METHOD(zbarcodeimage, __construct)
{
	php_zbarcode_image_object *intern;
	char *filename = NULL;
	int filename_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!", &filename, &filename_len) == FAILURE) {
		return;
	}
	
	if (!filename) {
		return;
	}

	intern        = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->image = _php_zbarcode_read(intern->magick_wand, filename);
	
	if (!intern->image) {
		PHP_ZBARCODE_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}
	return;
}
/* }}} */

/* {{{ zBarcodeImage zBarcodeImage::read(string filename)
	Read an image
*/
PHP_METHOD(zbarcodeimage, read)
{
	php_zbarcode_image_object *intern;
	char *filename;
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

	intern = (php_zbarcode_image_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (intern->image) {
		zbar_image_destroy(intern->image);
		intern->image = NULL;
	}
	intern->image = _php_zbarcode_read(intern->magick_wand, filename);
	
	if (!intern->image) {
		PHP_ZBARCODE_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}
	PHP_ZBARCODE_CHAIN_METHOD;
}
/* }}} */

/* {{{ array zBarCodeScanner::scan(zBarCodeImage image)
	Scan an image
*/
PHP_METHOD(zbarcodescanner, scan)
{
	php_zbarcode_scanner_object *intern;
	php_zbarcode_image_object   *intern_image;
	zval *object;
	const zbar_symbol_t *symbol;
	int n;
	zend_bool extended = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|b", &object, php_zbarcode_image_sc_entry, &extended) == FAILURE) {
		return;
	}
	
	intern = (php_zbarcode_scanner_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern_image = (php_zbarcode_image_object *)zend_object_store_get_object(object TSRMLS_CC);
	
	if (!intern_image->image) {
		zend_throw_exception(php_zbarcode_exception_class_entry, "The image is empty", 1 TSRMLS_CC);
		return;
	}
	
	array_init(return_value);

	/* scan the image for barcodes */
    n = zbar_scan_image(intern->scanner, intern_image->image);

    /* extract results */
    symbol = zbar_image_first_symbol(intern_image->image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
		zval *symbol_array, *loc_array;
		zbar_symbol_type_t symbol_type;
		const char *data;
		unsigned int loc_size, i;

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
		add_next_index_zval(return_value, symbol_array);
	}
	return;
}
/* }}} */

/* {{{ zBarCodeScanner zBarCodeScanner::setConfig(int name, int value[, int symbol])
	Set config option
*/
PHP_METHOD(zbarcodescanner, setconfig)
{
	php_zbarcode_scanner_object *intern;
	long symbol = 0, name, value;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|l", &name, &value, &symbol) == FAILURE) {
		return;
	}
	intern = (php_zbarcode_scanner_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zbar_image_scanner_set_config(intern->scanner, symbol, name, value) == 0) {
		zend_throw_exception(php_zbarcode_exception_class_entry, "Failed to set the configuration option", 1 TSRMLS_CC);
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
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(zbarcode_image_read_args, 0, 0, 1)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

static function_entry php_zbarcode_image_class_methods[] =
{
	PHP_ME(zbarcodeimage, __construct,	zbarcode_image_construct_args, 	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(zbarcodeimage, read,			zbarcode_image_read_args, 		ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

ZEND_BEGIN_ARG_INFO_EX(zbarcode_scanner_scan_args, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, zBarCodeImage, zBarCodeImage, 0) 
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
	
	if (intern->image) {
		zbar_image_destroy(intern->image);
		intern->image = NULL;
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
	intern->image       = NULL;
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
		Initialize the class (zbarcode)
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

#define PHP_BARDECODE_REGISTER_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(php_zbarcode_sc_entry, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC);

	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_ENABLE", ZBAR_CFG_ENABLE);			/**< enable symbology/feature */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_ADD_CHECK", ZBAR_CFG_ADD_CHECK);		/**< enable check digit when optional */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_EMIT_CHECK", ZBAR_CFG_EMIT_CHECK);	/**< return check digit when present */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_ASCII", ZBAR_CFG_ASCII);				/**< enable full ASCII character set */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_NUM", ZBAR_CFG_NUM);					/**< number of boolean decoder configs */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_MIN_LEN", ZBAR_CFG_MIN_LEN);			/**< minimum data length for valid decode */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_MAX_LEN", ZBAR_CFG_MAX_LEN);			/**< maximum data length for valid decode */
	
	/* Note, currently slightly hardcoded */
#ifdef HAVE_ZBAR_09
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_POSITION", ZBAR_CFG_POSITION);		/**< enable scanner to collect position data */
#endif
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_X_DENSITY", ZBAR_CFG_X_DENSITY);		/**< image scanner vertical scan density */
	PHP_BARDECODE_REGISTER_CONST_LONG("CFG_Y_DENSITY", ZBAR_CFG_Y_DENSITY);		/**< image scanner horizontal scan density */
               
	PHP_BARDECODE_REGISTER_CONST_LONG("SYM_NONE", ZBAR_NONE);			/**< no symbol decoded */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_PARTIAL", ZBAR_PARTIAL);		/**< intermediate status */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_EAN8", ZBAR_EAN8);			/**< EAN-8 */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_UPCE", ZBAR_UPCE);			/**< UPC-E */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_ISBN10", ZBAR_ISBN10);		/**< ISBN-10 (from EAN-13).*/
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_UPCA", ZBAR_UPCA);			/**< UPC-A */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_EAN13", ZBAR_EAN13);			/**< EAN-13 */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_ISBN13", ZBAR_ISBN13);		/**< ISBN-13 (from EAN-13).  */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_I25", ZBAR_I25);				/**< Interleaved 2 of 5. */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_CODE39", ZBAR_CODE39);		/**< Code 39.  */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_PDF417", ZBAR_PDF417);		/**< PDF417. */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_CODE128", ZBAR_CODE128);		/**< Code 128 */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_SYMBOL", ZBAR_SYMBOL);		/**< mask for base symbol type */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_ADDON2", ZBAR_ADDON2);		/**< 2-digit add-on flag */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_ADDON5", ZBAR_ADDON5);		/**< 5-digit add-on flag */
    PHP_BARDECODE_REGISTER_CONST_LONG("SYM_ADDON", ZBAR_ADDON);			/**< add-on flag mask */
      
#undef PHP_BARDECODE_REGISTER_CONST_LONG                                                        

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
	php_info_print_table_row(2, "zbar library version", zbar_ver);
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