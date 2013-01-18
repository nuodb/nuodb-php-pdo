--TEST--
Prepared statement with String parameter
--FILE--
<?php 
// Test prepared statement with String parameter.
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
  $position = 'Defense';
  $sql = "select count(*) from hockey where POSITION = :position";
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
Array
(
    [COUNT] => 8
    [0] => 8
)
done
