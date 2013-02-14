--TEST--
Test connection with a specified port number.
--FILE--
<?php 
try {  
  $db = new PDO("nuodb:database=test@localhost:48004;schema=PDOTEST", "dba", "goalie") or die;
  $sql = "select current_schema from dual";
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
    [] => PDOTEST
    [0] => PDOTEST
)
done
