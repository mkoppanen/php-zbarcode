--TEST--
Test setting config
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$scanner = new ZBarcodeScanner();
var_dump($scanner->setConfig(ZBarcode::CFG_MIN_LEN, 1));
var_dump($scanner->setConfig(ZBarcode::CFG_MIN_LEN, 10, ZBarcode::SYM_ALL));

var_dump($scanner->setConfig(ZBarcode::CFG_MAX_LEN, 50));
var_dump($scanner->setConfig(ZBarcode::CFG_MAX_LEN, 15, ZBarcode::SYM_ALL));

?>
--EXPECT--
object(zbarcodescanner)#1 (0) {
}
object(zbarcodescanner)#1 (0) {
}
object(zbarcodescanner)#1 (0) {
}
object(zbarcodescanner)#1 (0) {
}