--TEST--
Test construction
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$ZBarcodeImage = new ZBarcodeImage();
echo "OK\n";
$ZBarcodeScanner = new ZBarcodeScanner();
echo "OK\n";
?>
--EXPECT--
OK
OK