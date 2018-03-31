--TEST--
Test autocommit mode
--FILE--
<?php 

require("testdb.inc");

  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=PDOTEST";

// autocommit mode with explicit calls to beginTransction() and commit().
echo "autocommit enabled test with explicit calls to beginTransction() and commit().\n";
try {  
  $db = open_db_as_dba();

  // autocommit should be on by default			       
  if (1 !== ($tmp = $db->getAttribute(PDO::ATTR_AUTOCOMMIT)))
	printf("[001] Expecting int/1 got %s\n", var_export($tmp, true));

  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $db->beginTransaction();
  $sql4 = "select current_schema from dual";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  if (!$db->inTransaction()) echo "Failed inTransaction() test.\n";
  $db->commit();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  


// autocommit mode with explicit call to commit(), will 
// issue "There is no active transaction" error message.
echo "autocommit enabled test with explicit call to commit().\n";
try {  
  $db = open_db_as_dba();

  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $sql4 = "select current_schema from dual";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  $db->commit();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
}
$db = NULL;  

// autocommit mode with explicit call to rollback(), will 
// issue "There is no active transaction" error message.
echo "autocommit enabled test with explicit call to rollback().\n";
try {  
  $db = open_db_as_dba();

  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $sql4 = "select current_schema from dual";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  $db->rollback();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
}
$db = NULL;  


// auto commit disabled tests with commits and rollback.
echo "autocommit disabled test.\n";
try {  
  $db = new PDO($url, $dba_user, $dba_password, array(PDO::ATTR_AUTOCOMMIT => false)) or die;

  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $db->beginTransaction();
  $sql1 = "drop table char_test cascade if exists";
  $db->query($sql1);
  if (!$db->inTransaction()) echo "Failed inTransaction() test.\n";
  $db->commit();
  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $db->beginTransaction();
  $sql2 = "create table char_test (a char)";
  $db->query($sql2);
  $sql3 = "insert char_test (a) values('b')";
  $db->query($sql3);
  $db->commit();

  $db->beginTransaction();
  $sql4 = "select * from char_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  $db->commit();

  $db->beginTransaction();
  $sql5 = "insert char_test (a) values('c')";
  $db->query($sql5);
  $sql6 = "select * from char_test";
  $stmt = $db->prepare($sql6);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  $db->rollback();
  if ($db->inTransaction()) echo "Failed inTransaction() test.\n";

  $db->beginTransaction();
  $sql7 = "select * from char_test";
  $stmt = $db->prepare($sql7);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     print_r ($row);
  }
  $db->commit();

  $db->beginTransaction();
  $sql5 = "drop table char_test cascade if exists";
  $db->query($sql5);
  $db->commit();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

echo "done\n";
?>
--EXPECT--
autocommit enabled test with explicit calls to beginTransction() and commit().
Array
(
    [CURRENT_SCHEMA] => PDOTEST
    [0] => PDOTEST
)
autocommit enabled test with explicit call to commit().
Array
(
    [CURRENT_SCHEMA] => PDOTEST
    [0] => PDOTEST
)
There is no active transaction
autocommit enabled test with explicit call to rollback().
Array
(
    [CURRENT_SCHEMA] => PDOTEST
    [0] => PDOTEST
)
There is no active transaction
autocommit disabled test.
Array
(
    [A] => b
    [0] => b
)
Array
(
    [A] => b
    [0] => b
)
Array
(
    [A] => c
    [0] => c
)
Array
(
    [A] => b
    [0] => b
)
done
