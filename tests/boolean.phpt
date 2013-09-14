--TEST--
Test processing of boolean
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

  try {
      print "Dropping TEST table\n";
      $db->query("DROP TABLE TEST CASCADE IF EXISTS");

      print "Creating TEST table\n";
      $db->query("CREATE TABLE TEST (FLAG BOOLEAN NOT NULL DEFAULT '0')");

      print "Inserting a boolean\n";
      $flag = 0;
      $stmt = $db->prepare("INSERT INTO TEST (FLAG) VALUES (:flag)");
      $flag = 0;
      $stmt->bindParam(':flag', $flag, PDO::PARAM_BOOL);
      $stmt->execute();
      $flag = 1;
      $stmt->bindParam(':flag', $flag, PDO::PARAM_BOOL);
      $stmt->execute();

      print "Selecting boolean\n";
      $stmt = $db->prepare("SELECT * from test");
      $stmt->execute();
      $result = $stmt->fetchAll();
      foreach ($result as $row) 
      {
          echo "ROW = ";
	  if ($row[0] == true) {
             echo "1";
      	  } else {
            echo "0";
      	  }
      	  echo "\n";
      }
      // print_r($result);
    } catch (PDOException $e) {
      print $e;
    }
$db = NULL;
echo "\ndone\n";
?>
--EXPECT--
Dropping TEST table
Creating TEST table
Inserting a boolean
Selecting boolean
ROW = 0
ROW = 1

done
