PHP_ARG_WITH(zbarcode, whether to enable the zbarcode extension,
[ --with-zbarcode[=DIR]   Enables the zbarcode extension.], no)

PHP_ARG_WITH(zbarcode-imagemagick-dir, path to ImageMagick library,
[ --with-zbarcode-imagemagick-dir[=DIR]   path to ImageMagick library.], no)

if test $PHP_ZBARCODE != "no"; then

	AC_MSG_CHECKING(for zbar.h header)
	
	for i in $PHP_ZBARCODE /usr /usr/local /opt;
	do
		test -r $i/include/zbar.h && ZBAR_PATH=$i && break
	done
	
	if test -z "$ZBAR_PATH"; then
		AC_MSG_ERROR(Cannot locate zbar.h header. Please check your libzbar installation)
	else
		AC_MSG_RESULT(found in $ZBAR_PATH/include/zbar.h)
	fi
	
	PHP_CHECK_LIBRARY(zbar, zbar_image_scanner_create, [
		PHP_ADD_LIBRARY_WITH_PATH(zbar, $ZBAR_PATH/$PHP_LIBDIR, ZBARCODE_SHARED_LIBADD)
		PHP_ADD_INCLUDE($ZBAR_PATH/include)
	],[
		AC_MSG_ERROR([zbar library not found or error. Check config.log for details])
	],[
		-L$ZBAR_PATH/$PHP_LIBDIR
	])
	
	PHP_CHECK_LIBRARY(zbar, zbar_symbol_get_quality, [
		AC_DEFINE(HAVE_ZBAR_09,1,[ ])
	],[],[
		-L$ZBAR_PATH/$PHP_LIBDIR
	])
	
	AC_MSG_CHECKING(ImageMagick MagickWand API configuration program)
	
	for i in $PHP_ZBARCODE_IMAGEMAGICK_DIR /usr/local /usr /opt;
	do
		test -r $i/bin/MagickWand-config && WAND_BINARY=$i/bin/MagickWand-config && break
	done
	
	if test -z "$WAND_BINARY"; then
		for i in $PHP_ZBARCDODE_IMAGEMAGICK_DIR /usr/local /usr /opt;
		do
			test -r $i/bin/Wand-config && WAND_BINARY=$i/bin/Wand-config && ZBARCDODE_OLD_IM="true" && break
		done
	fi

	if test -z "$WAND_BINARY"; then
		AC_MSG_ERROR(not found. Please provide a path to MagickWand-config)
	else
		AC_MSG_RESULT(found in $WAND_BINARY)
	fi
	
	WAND_DIR="`$WAND_BINARY --prefix`"

	if test ! -z $ZBARCDODE_OLD_IM; then
	
		PHP_ADD_LIBRARY_WITH_PATH(Magick, $WAND_DIR/lib, ZBARCODE_SHARED_LIBADD)
		PHP_ADD_LIBRARY_WITH_PATH(Wand, $WAND_DIR/lib, ZBARCODE_SHARED_LIBADD)
		PHP_ADD_INCLUDE($WAND_DIR/include)	
	
		AC_DEFINE(ZBARCODE_IMAGEMAGICK_OLD,1,[ ])
	else	
		PHP_ADD_LIBRARY_WITH_PATH(MagickCore, $WAND_DIR/lib, ZBARCODE_SHARED_LIBADD)
		PHP_ADD_LIBRARY_WITH_PATH(MagickWand, $WAND_DIR/lib, ZBARCODE_SHARED_LIBADD)
		PHP_ADD_INCLUDE($WAND_DIR/include/ImageMagick)
	fi

	AC_DEFINE(HAVE_ZBARCODE,1,[ ])
	PHP_SUBST(ZBARCODE_SHARED_LIBADD)
	PHP_NEW_EXTENSION(zbarcode, zbarcode.c, $ext_shared)
fi
