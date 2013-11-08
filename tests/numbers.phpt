--TEST--
numbers test 
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {  

  $sql1 = "drop table numbers_test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table numbers_test (a INTEGER, b NUMBER, c NUMERIC, d BIGINT, e SMALLINT, f DECIMAL, g DEC, h INT, i NUMERIC(21) )";
  $db->query($sql2);

  $sql3 = "insert numbers_test (a, b, c, d, e, f, g, h, i) values('1', '2', '3', '4', '5', '6', '7', '8', '9')";
  $db->query($sql3);

  $sql4 = "select * from numbers_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }

  $sql5 = "drop table numbers_test cascade if exists";
  $db->query($sql5);

  $sql6 = "create table numbers_test (i REAL, j FLOAT, k DOUBLE PRECISION)";
  $db->query($sql6);

  $sql7 = "insert numbers_test (i, j, k) values('1.2', '3.4', '5.6')";
  $db->query($sql7);

  $sql8 = "select * from numbers_test";
  $stmt = $db->prepare($sql8);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }

  $sql9 = "drop table numbers_test cascade if exists";
  $db->query($sql9);

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "done\n";
?>
--EXPECT--
Array
(
    [A] => 1
    [0] => 1
    [B] => 2
    [1] => 2
    [C] => 3
    [2] => 3
    [D] => 4
    [3] => 4
    [E] => 5
    [4] => 5
    [F] => 6
    [5] => 6
    [G] => 7
    [6] => 7
    [H] => 8
    [7] => 8
    [I] => 9
    [8] => 9
)
Array
(
    [I] => 1.2
    [0] => 1.2
    [J] => 3.4
    [1] => 3.4
    [K] => 5.6
    [2] => 5.6
)
done
