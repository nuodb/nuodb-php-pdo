--TEST--
Prepared statement with NULL string parameter
--FILE--
<?php
// Test prepared statement with NULL string parameter. 
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
  $position = NULL;
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
    [COUNT] => 0
    [0] => 0
)
done

