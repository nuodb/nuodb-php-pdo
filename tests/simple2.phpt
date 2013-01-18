--TEST--
Simple2 test using the Hockey database with a non-existing table named test1.
--FILE--
<?php 
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
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
