--TEST--
Test construction
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$zBarcodeImage = new zBarcodeImage();
echo "OK\n";
$zBarcodeScanner = new zBarcodeScanner();
echo "OK\n";
?>
--EXPECT--
OK
OK