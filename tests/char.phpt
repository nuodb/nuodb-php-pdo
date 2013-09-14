--TEST--
char test 
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {  

  $sql1 = "drop table char_test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table char_test (a char)";
  $db->query($sql2);

  $sql3 = "insert char_test (a) values('b')";
  $db->query($sql3);

  $sql4 = "select * from char_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }

  $sql5 = "drop table char_test cascade if exists";
  $db->query($sql5);

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
    [A] => b
    [0] => b
)
done