--TEST--
Prepared statement with small Integer parameter
--FILE--
<?php 
// Test prepared statement with small Integer parameter.
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=drupal", "cloud", "user") or die;
//  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
  $sql = "DROP TABLE TEST CASCADE IF EXISTS";
  $stmt = $db->prepare($sql);
  $stmt->execute();
  $sql = "CREATE TABLE TEST (NAME VARCHAR(255) NOT NULL DEFAULT '', NUM SMALLINT NOT NULL DEFAULT -1)";
  $stmt = $db->prepare($sql);
  $stmt->execute();
  $name = 'foo';
  $sql = "INSERT INTO TEST (NAME) VALUES (:name)";
  $stmt = $db->prepare($sql);

  $stmt->bindParam(':name', $name, PDO::PARAM_STR);
  $stmt->execute();

  $sql = "SELECT NAME, NUM FROM TEST WHERE NAME = :name";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':name', $name, PDO::PARAM_STR);
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
    [NAME] => foo
    [0] => foo
    [NUM] => -1
    [1] => -1
)
done