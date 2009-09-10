--TEST--
Test count
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$image = new ZBarcodeImage();
$image->read(dirname(__FILE__) . "/ean13.jpg")
      ->read(dirname(__FILE__) . "/ean13.jpg")
      ->read(dirname(__FILE__) . "/ean13.jpg")
      ->read(dirname(__FILE__) . "/ean13.jpg");

echo "Count: " . $image->count() . "\n";
$image->clear();
echo "Count: " . $image->count() . "\n";

?>
--EXPECT--
Count: 4
Count: 0