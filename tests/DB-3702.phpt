--TEST--
DB-3702 converting decimal
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {  
  $sql = "drop table db_3702 if exists";
  $db->query($sql);
  $sql = "create table db_3702 (a decimal(11,4))";
  $db->query($sql);
  $sql = "insert into db_3702(a) values (36.9500)";
  $db->query($sql);
  $sql = "select a from db_3702";
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
    [A] => 36.95
    [0] => 36.95
)
done
