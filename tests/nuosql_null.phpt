--TEST--
The NUOSQL_NULL
--FILE--
<?php 

require("testdb.inc");
global $db;  

try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  $sql = "SELECT NULL FROM DUAL";
  $stmt = $db->prepare($sql);
  $stmt->execute();
  $result = $stmt->fetchAll();

  foreach ($result as $row) {
     if ($row[0] !== null) {
        print("FAIL: Did not recieve the expected null value");
     }
     print_r ($row);
  }

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
echo "done\n";
?>
--EXPECT--
Array
(
    [] => 
    [0] => 
)
done
