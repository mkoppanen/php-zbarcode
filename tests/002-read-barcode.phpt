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

$info = $scanner->scan($image, 0, true);
echo gettype($info[1][0]['location']) . "\n";

/* Try to scan empty object */
$image->clear();
try {
var_dump($scanner->scan($image));
} catch (ZBarCodeException $e) {
	echo "Got exception";
}
?>
--EXPECTF--
object(zbarcodeimage)#%d (0) {
}
-----------------------------
array(1) {
  [0]=>
  array(3) {
    ["data"]=>
    string(13) "5901234123457"
    ["type"]=>
    string(6) "EAN-13"
    ["quality"]=>
    int(539)
  }
}
array
Got exception