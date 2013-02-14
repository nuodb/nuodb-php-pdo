--TEST--
Prepared statement with both Integer and String parameters.
--FILE--
<?php 

require("testdb.inc");
global $db;

// Test Prepared statement with both Integer and String parameters.
try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  // create table Hockey_test
  drop_table_hockey_test();
  create_table_hockey_test();

  $player_number = 30;
  $position = "Forward";
  $sql = "select count(*) from hockey_test where NUMBER < :number and POSITION = :position";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':number', $player_number, PDO::PARAM_INT);
  $stmt->bindParam(':position', $position, PDO::PARAM_STR);
  $stmt->execute();
  $result = $stmt->fetchAll();
  print "There are " . $result[0]["COUNT"] . " " . $position . " players with numbers less than " . $player_number . "\n";
  $player_number = 30;
  $position = "Defense";
  $stmt->execute();
  $result = $stmt->fetchAll();
  print "There are " . $result[0]["COUNT"] . " " . $position . " players with numbers less than " . $player_number . "\n";
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "done\n";
?>
--EXPECT--
drop table Hockey_test
create table Hockey_test
There are 7 Forward players with numbers less than 30
There are 2 Defense players with numbers less than 30
done
