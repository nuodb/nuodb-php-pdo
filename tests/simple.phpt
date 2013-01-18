--TEST--
Simple test using the Hockey database.
--FILE--
<?php 
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $sql = "select * from hockey where NUMBER<12";
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
    [ID] => 2
    [0] => 2
    [NUMBER] => 11
    [1] => 11
    [NAME] => GREGORY CAMPBELL
    [2] => GREGORY CAMPBELL
    [POSITION] => Forward
    [3] => Forward
    [TEAM] => Bruins
    [4] => Bruins
)
Array
(
    [ID] => 25
    [0] => 25
    [NUMBER] => 1
    [1] => 1
    [NAME] => MAX SUMMIT
    [2] => MAX SUMMIT
    [POSITION] => Fan
    [3] => Fan
    [TEAM] => Bruins
    [4] => Bruins
)
done

