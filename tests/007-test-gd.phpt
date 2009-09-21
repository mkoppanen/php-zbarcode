--TEST--
Test GD support
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";

if (!extension_loaded('gd')) {
	die("Skip GD extension is not loaded");
}

if (ZBarCode::HAVE_GD !== true) {
	die("Skip GD support not enabled");
}
?>
--FILE--
<?php
$image = imagecreatefromjpeg(dirname(__FILE__) . '/ean13.jpg');

$scanner = new ZbarCodeScanner();
var_dump($scanner->scan($image));
?>
--EXPECT--
array(1) {
  [1]=>
  array(1) {
    [0]=>
    array(2) {
      ["data"]=>
      string(13) "5901234123457"
      ["type"]=>
      string(6) "EAN-13"
    }
  }
}