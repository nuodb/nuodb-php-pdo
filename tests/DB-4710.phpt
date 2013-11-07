--TEST--
DB-4710 -- insert test
--FILE--
<?php 
date_default_timezone_set('America/New_York');

require("testdb.inc");
global $db;  
open_db();

try {  
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  $sql0 = 'DROP TABLE gentest CASCADE IF EXISTS';
  $db->query($sql0);

  $sql1 = 'create table gentest (id int generated always as identity, data string);';
  $db->query($sql1);

  $sql2 = "insert into gentest (id, data) values (null, 'asdf')";
  $status = $db->exec($sql2);
  var_dump($status);

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "\ndone\n";
?>
--EXPECT--
SQLSTATE[42000]: Syntax error or access violation: -1 can't assign a value to generated always identity "ID"
SQL: insert into gentest (id, data) values (null, 'asdf')
done
