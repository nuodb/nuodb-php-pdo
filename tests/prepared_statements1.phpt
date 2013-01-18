--TEST--
Prepared statement with Integer parameter
--FILE--
<?php 
// Test prepared statement with Integer parameter.
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
  $player_number = 30;
  $sql = "select * from hockey where NUMBER = :number";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':number', $player_number, PDO::PARAM_INT);
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
Array
(
    [ID] => 24
    [0] => 24
    [NUMBER] => 30
    [1] => 30
    [NAME] => TIM THOMAS
    [2] => TIM THOMAS
    [POSITION] => Goalie
    [3] => Goalie
    [TEAM] => Bruins
    [4] => Bruins
)
done
