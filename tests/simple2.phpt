--TEST--
Simple2 test using a non-existing table named test1.
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {  
  $sql = "select * from test1 where t=1234567";
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
SQLSTATE[HY000] [-25] can't find table "TEST1"
SQL: select * from test1 where t=1234567done
