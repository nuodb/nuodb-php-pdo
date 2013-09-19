--TEST--
Prepared statements with positional parameters
--FILE--
<?php 

require("testdb.inc");
global $db;

// Test prepared statements with positional parameters.
try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  // create table Hockey_test
  drop_table_hockey_test();
  create_table_hockey_test();

  $sql = "select * from hockey_test where NUMBER = ?";
  $stmt = $db->prepare($sql);

  $player_number = 23;
  $stmt->bindParam(1, $player_number, PDO::PARAM_INT);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     echo "NUMBER=" . $row["NUMBER"] . " NAME=" . $row["NAME"] . " POSITION=" . $row["POSITION"] . " TEAM=" . $row["TEAM"] . "\n";
     //print_r ($row);
  }

  $player_number = 33;
  $stmt->bindParam(1, $player_number, PDO::PARAM_INT);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     echo "NUMBER=" . $row["NUMBER"] . " NAME=" . $row["NAME"] . " POSITION=" . $row["POSITION"] . " TEAM=" . $row["TEAM"] . "\n";
     //print_r ($row);
  }

  $number = 99;
  $name = "Mickey Mouse";
  $position = "Defense";
  $team = "Disney";
  $sql = "INSERT INTO Hockey_test (Number, Name, Position, Team) VALUES (?, ?, ?, ?)";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(1, $number, PDO::PARAM_INT);
  $stmt->bindParam(2, $name, PDO::PARAM_STR);
  $stmt->bindParam(3, $position, PDO::PARAM_STR);
  $stmt->bindParam(4, $team, PDO::PARAM_STR);
  $stmt->execute();
  
  $sql = "select * from hockey_test where NUMBER = ?";
  $stmt = $db->prepare($sql);
  $player_number = 99;
  $stmt->bindParam(1, $player_number, PDO::PARAM_INT);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     echo "NUMBER=" . $row["NUMBER"] . " NAME=" . $row["NAME"] . " POSITION=" . $row["POSITION"] . " TEAM=" . $row["TEAM"] . "\n";
     //print_r ($row);
  }
  
} catch(PDOException $e) {  
  echo $e->getMessage();  
}

// test exception raised by an invalid parameter number
try {
  $sql = "select * from hockey_test where NUMBER = ?";
  $stmt = $db->prepare($sql);
  $player_number = 99;
  $stmt->bindParam(1, $player_number, PDO::PARAM_INT);
  $stmt->bindParam(2, $player_number, PDO::PARAM_INT);  // this one is not valid
  $stmt->execute();
} catch (PDOException $e) {
  echo "Got expected exception: " . $e->getMessage() . "\n";  
}


$db = NULL;  
echo "done\n";
?>
--EXPECT--
drop table Hockey_test
create table Hockey_test
NUMBER=23 NAME=CHRIS KELLY POSITION=Forward TEAM=Bruins
NUMBER=33 NAME=ZDENO CHARA POSITION=Defense TEAM=Bruins
NUMBER=99 NAME=Mickey Mouse POSITION=Defense TEAM=Disney
Got expected exception: SQLSTATE[HY093]: Invalid parameter number: -12 Invalid parameter number 1
done
