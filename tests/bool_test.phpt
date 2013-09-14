--TEST--
boolean tests
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try 
{  
  $db->exec('DROP TABLE IF EXISTS bool_test');
  $db->exec('CREATE TABLE bool_test(id INT, mybool BOOLEAN)');

  $id = 1;
  $mybool = false;
  var_dump($mybool);

  $stmt = $db->prepare('INSERT INTO bool_test(id, mybool) VALUES (?, ?)');
  $stmt->bindParam(1, $id);
  $stmt->bindParam(2, $mybool, PDO::PARAM_BOOL);
  var_dump($mybool);

  $stmt->execute();
  var_dump($mybool);

  $mybool = true;
  var_dump($mybool);

  $stmt->execute();
  var_dump($mybool);

  echo "Select Result 1:\n";
  $stmt = $db->query('SELECT * FROM bool_test');
  var_dump($stmt->fetchAll(PDO::FETCH_ASSOC));

  $mybool = false;
  var_dump($mybool);

  $stmt = $db->prepare('INSERT INTO bool_test(id, mybool) VALUES (?, ?)');
  $stmt->bindParam(1, $id);

  // INT and integer work well together
  $stmt->bindParam(2, $mybool, PDO::PARAM_INT);
  $stmt->execute();

  $mybool = true;
  var_dump($mybool);
  $stmt->execute();

  echo "Select Result 2:\n";
  $stmt = $db->query('SELECT * FROM bool_test');
  var_dump($stmt->fetchAll(PDO::FETCH_ASSOC));

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}

$db = NULL;  
echo "done\n";

?>
--EXPECT--
bool(false)
bool(false)
bool(false)
bool(true)
bool(true)
Select Result 1:
array(2) {
  [0]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(false)
  }
  [1]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(true)
  }
}
bool(false)
bool(true)
Select Result 2:
array(4) {
  [0]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(false)
  }
  [1]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(true)
  }
  [2]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(false)
  }
  [3]=>
  array(2) {
    ["ID"]=>
    string(1) "1"
    ["MYBOOL"]=>
    bool(true)
  }
}
done
