--TEST--
Prepared statement with NULL string parameter
--FILE--
<?php
// Test prepared statement with NULL string parameter. 

require("testdb.inc");
global $db;

try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  // create table Hockey_test
  drop_table_hockey_test();
  create_table_hockey_test();

  $position = NULL;
  $sql = "select count(*) from hockey_test where POSITION = :position";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':position', $position, PDO::PARAM_STR);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
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
drop table Hockey_test
create table Hockey_test
Array
(
    [COUNT] => 0
    [0] => 0
)
done

