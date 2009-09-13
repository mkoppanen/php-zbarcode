--TEST--
Check whether imagick integration works
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";

if (!extension_loaded('imagick')) {
	die("Skip Imagick extension is not loaded");
}

if (ZBarCode::HAVE_IMAGICK !== true) {
	die("Skip Imagick support not enabled");
}

?>
--FILE--
<?php
$image = new Imagick(dirname(__FILE__) . "/ean13.jpg");
$image2 = new ZBarCodeImage(dirname(__FILE__) . "/ean13.jpg");

$scanner = new ZBarCodeScanner();
var_dump(array_diff($scanner->scan($image), $scanner->scan($image)));

?>
--EXPECT--
array(0) {
}