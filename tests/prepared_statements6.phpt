--TEST--
Prepared statement with Integer parameter for NuoDB Timestamp column.
--FILE--
<?php 
// Test prepared statement with Integer parameter for NuoDB Timestamp column.
date_default_timezone_set('America/New_York');

require("testdb.inc");
global $db;  
open_db();

try {  
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
  $date = '1962-12-28';
  $sql = "select * from test1 where d = :date";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':date', $date, PDO::PARAM_STR);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
     print $row[0] . " " . $row[1] . " " . $row[2] . " " . date('Y-m-d', $row[3]) . " " . date('H:i:s', $row[4]) . "\n";
  }
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "\ndone\n";
?>
--EXPECT--
SQLSTATE[42000]: Syntax error or access violation: -25 can't find table "TEST1"
SQL: select * from test1 where d = ?
done
