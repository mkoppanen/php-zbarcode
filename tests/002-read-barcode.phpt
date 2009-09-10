--TEST--
Test reading image and scanning barcode
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$image = new ZBarcodeImage();
var_dump($image->read(dirname(__FILE__) . "/ean13.jpg"));

echo "-----------------------------\n";

$scanner = new ZBarcodeScanner();
var_dump($scanner->scan($image));

$info = $scanner->scan($image, true);

if (isset($info[0]['location']))
	echo "Got location";
?>
--EXPECT--
object(zbarcodeimage)#1 (0) {
}
-----------------------------
array(1) {
  [0]=>
  array(2) {
    ["data"]=>
    string(13) "5901234123457"
    ["type"]=>
    string(6) "EAN-13"
  }
}
Got location