--TEST--
test quote for null byte.
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {


  $sql1 = "drop table char_test cascade if exists";
  $db->query($sql1);
   
  $sql2 = "create table char_test (i int,a varchar(100))";
  $db->query($sql2);

  $string1 = 'This is not a Null value'; 
  $sql3 = sprintf("insert char_test (i,a) values(100,%s)",$db->quote($string1));
  $db->query($sql3);

  $string2 = 'This is a Null \0 value test in PHP string';
  $sql3 = sprintf("insert char_test (i,a) values(200,%s)",$db->quote($string2));
  $db->query($sql3);  

  $sql4 = "select * from char_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
}


} catch(PDOException $e) {
  echo $e->getMessage();
}
$db = NULL;
echo "done\n";
?>
--EXPECT--
Array
(
    [I] => 100
    [0] => 100
    [A] => This is not a Null value
    [1] => This is not a Null value
)
Array
(
    [I] => 200
    [0] => 200
    [A] => This is a Null \0 value test in PHP string
    [1] => This is a Null \0 value test in PHP string
)
done
