--TEST--
Prepared statement with positional Integer parameter
--FILE--
<?php 

require("testdb.inc");
global $db;


// Test prepared statement with Integer parameter.
try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  // create table Hockey_test
  drop_table_hockey_test();
  create_table_hockey_test();

  $player_number = 33;
  $sql = "select * from hockey_test where NUMBER = ?";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(1, $player_number, PDO::PARAM_INT);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     echo "NUMBER=" . $row["NUMBER"] . " NAME=" . $row["NAME"] . " POSITION=" . $row["POSITION"] . " TEAM=" . $row["TEAM"] . "\n";
     //print_r ($row);
  }
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "done\n";
?>
--EXPECT--
drop table Hockey_test
create table Hockey_test
NUMBER=33 NAME=ZDENO CHARA POSITION=Defense TEAM=Bruins
done
