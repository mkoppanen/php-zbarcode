PHP_ARG_WITH(zbarcode, whether to enable the zbarcode extension,
[  --with-zbarcode[=DIR]   Enables the zbarcode extension.], no)

PHP_ARG_WITH(zbarcode-imagemagick-dir, path to ImageMagick library,
[  --with-zbarcode-imagemagick-dir[=DIR]   path to ImageMagick library.], no)

PHP_ARG_ENABLE(zbarcode-imagick, whether to enable zbarcode Imagick support,
[  --enable-zbarcode-imagick     whether to enable zbarcode Imagick support], no, no)

PHP_ARG_ENABLE(zbarcode-gd, whether to enable zbarcode GD Imagick support,
[  --enable-zbarcode-gd     whether to enable zbarcode GD support], no, no)

if test $PHP_ZBARCODE != "no"; then

#
# ImageMagick macros
#
  m4_include([imagemagick.m4])
  IM_FIND_IMAGEMAGICK(6002004, $PHP_ZBARCODE_IMAGEMAGICK_DIR)

  AC_MSG_CHECKING(zbar installation)
  if test "x$PHP_ZBARCODE" = "xyes"; then
    if test "x${PKG_CONFIG_PATH}" = "x"; then
      #
      # "By default, pkg-config looks in the directory prefix/lib/pkgconfig for these files"
      #
      # Add a bit more search paths for common installation locations. Can be overridden by setting
      # PKG_CONFIG_PATH env variable or passing --with-zbarcode=PATH
      #
      export PKG_CONFIG_PATH="/usr/local/${PHP_LIBDIR}/pkgconfig:/usr/${PHP_LIBDIR}/pkgconfig:/opt/${PHP_LIBDIR}/pkgconfig:/opt/local/${PHP_LIBDIR}/pkgconfig"
    fi
  else
    export PKG_CONFIG_PATH="${PHP_ZBARCODE}/${PHP_LIBDIR}/pkgconfig"
  fi

  if $PKG_CONFIG --exists zbar; then
    PHP_ZBAR_VERSION=`$PKG_CONFIG zbar --modversion`
    PHP_ZBAR_PREFIX=`$PKG_CONFIG zbar --variable=prefix`

    AC_MSG_RESULT([found version $PHP_ZMQ_VERSION, under $PHP_ZBAR_PREFIX])
    PHP_ZBAR_LIBS=`$PKG_CONFIG zbar --libs`
    PHP_ZBAR_INCS=`$PKG_CONFIG zbar --cflags`

    PHP_EVAL_LIBLINE($PHP_ZBAR_LIBS, ZBARCODE_SHARED_LIBADD)
    PHP_EVAL_INCLINE($PHP_ZBAR_INCS)
  else
    AC_MSG_ERROR(Unable to find zbar installation)
  fi


  PHP_CHECK_LIBRARY(zbar, zbar_symbol_get_quality, [
    AC_DEFINE(HAVE_ZBAR_SYMBOL_GET_QUALITY,1,[ ])
  ],[],[
    -L$PHP_ZBAR_PREFIX/$PHP_LIBDIR
  ])

#
# Imagick support
#
  if test $PHP_ZBARCODE_IMAGICK != "no"; then
    AC_MSG_CHECKING(php_imagick_shared.h header file)
  
    if test -z "$PHP_CONFIG"; then
        AC_MSG_ERROR([php-config not found])
      fi
  
    PHP_IMAGICK_HEADER="`$PHP_CONFIG --include-dir`/ext/imagick/php_imagick_shared.h"
    
    if test -r $PHP_IMAGICK_HEADER; then
      AC_MSG_RESULT(found.)
      AC_DEFINE(HAVE_ZBARCODE_IMAGICK,1,[ ])
      
       PHP_ADD_EXTENSION_DEP(zbarcode, imagick)
    else
      AC_MSG_ERROR(not found. Run with --disable-zbarcode-imagick to disable this feature)
    fi
  fi

#
# GD support
#
  if test $PHP_ZBARCODE_GD != "no"; then

    AC_MSG_CHECKING(ext/gd/php_gd.h header file)
    
    if test -z "$PHP_CONFIG"; then
        AC_MSG_ERROR([php-config not found])
      fi
  
    PHP_GD_CHECK_HEADER="`$PHP_CONFIG --include-dir`/ext/gd/php_gd.h"
  
    if test -r $PHP_GD_CHECK_HEADER; then
      AC_MSG_RESULT(found.)
    else
      AC_MSG_ERROR(not found. Run with --disable-zbarcode-gd to disable this feature)
    fi

    PHP_ADD_EXTENSION_DEP(zbarcode, gd)
    AC_DEFINE(HAVE_ZBARCODE_GD,1,[ ])
  fi  

  PHP_EVAL_LIBLINE($IM_IMAGEMAGICK_LIBS, ZBARCODE_SHARED_LIBADD)
  PHP_EVAL_INCLINE($IM_IMAGEMAGICK_CFLAGS)

  AC_DEFINE(HAVE_ZBARCODE,1,[ ])
  PHP_SUBST(ZBARCODE_SHARED_LIBADD)
  PHP_NEW_EXTENSION(zbarcode, zbarcode.c, $ext_shared,,$IM_IMAGEMAGICK_CFLAGS)
fi
