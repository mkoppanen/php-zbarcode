--TEST--
Test multipage reading and single page reading
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

$scanner = new ZBarcodeScanner();
var_dump($scanner->scan($image, 0));

echo "-----------------------------\n";

var_dump($scanner->scan($image, 4));

echo "-----------------------------\n";

$image->clear();
$image->read(dirname(__FILE__) . "/ean13.jpg");
var_dump($scanner->scan($image));

?>
--EXPECT--
array(4) {
  [1]=>
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
  [2]=>
  array(1) {
    [0]=>
    array(3) {
      ["data"]=>
      string(13) "5901234123457"
      ["type"]=>
      string(6) "EAN-13"
      ["quality"]=>
      int(540)
    }
  }
  [3]=>
  array(1) {
    [0]=>
    array(3) {
      ["data"]=>
      string(13) "5901234123457"
      ["type"]=>
      string(6) "EAN-13"
      ["quality"]=>
      int(540)
    }
  }
  [4]=>
  array(1) {
    [0]=>
    array(3) {
      ["data"]=>
      string(13) "5901234123457"
      ["type"]=>
      string(6) "EAN-13"
      ["quality"]=>
      int(540)
    }
  }
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
    int(540)
  }
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
    int(540)
  }
}