--TEST--
Test processing of special characters.
--FILE--
<?php 
  $db = new PDO("nuodb:database=test@localhost", "dba", "goalie") or die;
  try {
      print "Dropping TEST table\n";
      $db->query("DROP TABLE TEST CASCADE IF EXISTS");

      print "Creating TEST table\n";
      $db->query("CREATE TABLE test(name STRING NOT NULL)");

      print "Inserting a string with a special char\n";
      $name = "Martín Lucas";
      $stmt = $db->prepare("INSERT INTO test (name) VALUES (:name)");
      $stmt->bindParam(':name', $name, PDO::PARAM_STR);
      $stmt->execute();

      print "Selecting a string with a special char\n";
      $stmt = $db->prepare("SELECT * from test");
      $stmt->execute();
      $result = $stmt->fetchAll();
      print_r($result);
    } catch (PDOException $e) {
      print $e;
    }
$db = NULL;
echo "\ndone\n";
?>
--EXPECT--
Dropping TEST table
Creating TEST table
Inserting a string with a special char
Selecting a string with a special char
Array
(
    [0] => Array
        (
            [NAME] => Martín Lucas
            [0] => Martín Lucas
        )

)

done

