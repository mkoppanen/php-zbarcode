Simple extension to read barcodes from images

[![Build Status](https://travis-ci.org/mkoppanen/php-zbarcode.svg?branch=master)](https://travis-ci.org/mkoppanen/php-zbarcode)

## Upgrade at 2022-03-11

Support libzbar2*, And add code point out.

## Upgrade at 2018-12-05

Support PHP7, And fix some bug that may cause crash.

## Basic usage:

    <?php
    /* Create new image object */
    $image = new ZBarCodeImage("test.jpg");

    /* Create a barcode scanner */
    $scanner = new ZBarCodeScanner();

    /* Scan the image */
    $barcode = $scanner->scan($image);

    /* Loop through possible barcodes */
    if (!empty($barcode)) {
    	foreach ($barcode as $code) {
    		printf("Found type %s barcode with data %s\n", $code['type'], $code['data']);
    	}
    }
    ?>

The EAN13 image in the tests/ directory is from 
http://en.wikipedia.org/wiki/File:Ean-13-5901234123457.png

## Dependencies:

ZBar 
http://zbar.sourceforge.net/

ImageMagick
http://www.imagemagick.org/
