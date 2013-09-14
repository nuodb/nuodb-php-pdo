--TEST--
Simple test.
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {  

  $sql = "DROP TABLE SIMPLE CASCADE IF EXISTS";
  $db->query($sql);
  
  $sql = "CREATE TABLE SIMPLE (NAME VARCHAR(255) NOT NULL DEFAULT '', NUM INTEGER NOT NULL DEFAULT -1)";
  $db->query($sql);

  $sql = "INSERT INTO SIMPLE (NAME, NUM) VALUES ('Rick', 1)";
  $db->query($sql);

  $sql = "INSERT INTO SIMPLE (NAME, NUM) VALUES ('Bill', 2)";
  $db->query($sql);
  
  $sql = "INSERT INTO SIMPLE (NAME, NUM) VALUES ('Tom', 3)";
  $db->query($sql);

  $sql = "INSERT INTO SIMPLE (NAME, NUM) VALUES ('Bob', 4)";
  $db->query($sql);

  $sql = "INSERT INTO SIMPLE (NAME, NUM) VALUES ('Dave', 5)";
  $db->query($sql);

  $sql = "SELECT * FROM SIMPLE";
  foreach ($db->query($sql) as $row) {
     print_r ($row);
  }
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
    [NAME] => Rick
    [0] => Rick
    [NUM] => 1
    [1] => 1
)
Array
(
    [NAME] => Bill
    [0] => Bill
    [NUM] => 2
    [1] => 2
)
Array
(
    [NAME] => Tom
    [0] => Tom
    [NUM] => 3
    [1] => 3
)
Array
(
    [NAME] => Bob
    [0] => Bob
    [NUM] => 4
    [1] => 4
)
Array
(
    [NAME] => Dave
    [0] => Dave
    [NUM] => 5
    [1] => 5
)
done
