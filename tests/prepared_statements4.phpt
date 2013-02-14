--TEST--
Test insert prepared statements.
--FILE--
<?php 
// Test insert prepared statements.

require("testdb.inc");
global $db;


try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  // create table Hockey_test
  drop_table_hockey_test();
  create_table_hockey_test();

  $position = "Fan";
  $sql = "insert into Hockey_test(ID, NUMBER, NAME, POSITION, TEAM) values (999, 101, 'Tom Gates', 'Fan', 'Bruins')";  // Insert ID to workaround DB-2386.
  $stmt = $db->prepare($sql);
  $stmt->execute();
  $sql = "select count(*) from Hockey_test where POSITION = :position";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':position', $position, PDO::PARAM_STR);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
drop_table_hockey_test();
$db = NULL;  
echo "done\n";
?>
--EXPECT--
drop table Hockey_test
create table Hockey_test
Array
(
    [COUNT] => 2
    [0] => 2
)
drop table Hockey_test
done
