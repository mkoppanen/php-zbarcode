--TEST--
Test setting config
--SKIPIF--
<?php
include dirname(__FILE__) . "/skipif.inc.php";
?>
--FILE--
<?php
$scanner = new zBarcodeScanner();
var_dump($scanner->setConfig(zBarcode::CFG_MIN_LEN, 1));
var_dump($scanner->setConfig(zBarcode::CFG_MIN_LEN, 10, zBarcode::SYM_ALL));

var_dump($scanner->setConfig(zBarcode::CFG_MAX_LEN, 50));
var_dump($scanner->setConfig(zBarcode::CFG_MAX_LEN, 15, zBarcode::SYM_ALL));

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