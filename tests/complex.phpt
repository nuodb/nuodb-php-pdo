--TEST--
Complex test using the Hockey table
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

function query_with_row_count($sql) {
  $row_count = 0;
  try {  
    $db = open_db();
    foreach ($db->query($sql) as $row) {
      // print_r($row);
      $row_count++;
    }
    $db = NULL;
  } catch(PDOException $e) {  
    echo $e->getMessage();  
  }
  $db = NULL;  
  return $row_count;
}

// create table Hockey_test
drop_table_hockey_test();
create_table_hockey_test();



// select test1
print("select test1\n");
$sql = "select * from Hockey_test where NUMBER<12";
$row_count = query_with_row_count($sql);
if ($row_count != 2) {
   print("FAILED row count: $sql\nExpected 2, got $row_count\n");
}

// select test2
print("select test2\n");
$sql = "select * from Hockey_test";
$row_count = query_with_row_count($sql);
if ($row_count != 24) {
   print("FAILED row count: $sql\nExpected 24, got $row_count\n");
}


// insert test1
print("insert test1\n");
try {  
  $db = open_db();
  $sql = "INSERT INTO Hockey_test(ID, NUMBER, NAME, POSITION, TEAM) VALUES(999, '99', 'Mickey Mouse', 'Center', 'Disney')";  // Insert ID to workaround DB-2386.
  $count = $db->exec($sql);
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test3
print("select test3\n");
$sql = "select * from Hockey_test";
$row_count = query_with_row_count($sql);
if ($row_count != 25) {
   print("FAILED row count: $sql\nExpected 25, got $row_count\n");
}

// delete test1
print("delete test1\n");
try {  
  $db = open_db();
  $sql = "DELETE FROM Hockey_test WHERE TEAM='Disney'";
  $count = $db->exec($sql);
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test4
print("select test4\n");
$sql = "select * from Hockey_test";
$row_count = query_with_row_count($sql);
if ($row_count != 24) {
   print("FAILED row count: $sql\nExpected 24, got $row_count\n");
}
$db = NULL;

// select test5 - query and fetch with FETCH_ASSOC
print("select test5\n");
$sql = "select * from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  $result = $stmt->fetch(PDO::FETCH_ASSOC);
  if ($result["NUMBER"] != "37") {
     print("FAILED: select test5\n");
  }
  if ($result["NAME"] != "PATRICE BERGERON") {
     print("FAILED: select test5\n");
  }
  if ($result["POSITION"] != "Forward") {
     print("FAILED: select test5\n");
  }
  if ($result["TEAM"] != "Bruins") {
     print("FAILED: select test5\n");
  }
  $stmt = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test6 - query and fetch with FETCH_NUM
print("select test6\n");
$sql = "select * from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  $result = $stmt->fetch(PDO::FETCH_NUM);
  if ($result[1] != "37") {
     print("FAILED: select test6\n");
  }
  if ($result[2] != "PATRICE BERGERON") {
     print("FAILED: select test6\n");
  }
  if ($result[3] != "Forward") {
     print("FAILED: select test6\n");
  }
  if ($result[4] != "Bruins") {
     print("FAILED: select test6\n");
  }
  $stmt = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  


// select test7 - query and fetch with FETCH_BOTH
print("select test7\n");
$sql = "select * from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  $result = $stmt->fetch(PDO::FETCH_BOTH);
  if ($result[1] != "37") {
     print("FAILED: select test7\n");
  }
  if ($result[2] != "PATRICE BERGERON") {
     print("FAILED: select test7\n");
  }
  if ($result[3] != "Forward") {
     print("FAILED: select test7\n");
  }
  if ($result[4] != "Bruins") {
     print("FAILED: select test7\n");
  }
  if ($result["NUMBER"] != "37") {
     print("FAILED: select test7\n");
  }
  if ($result["NAME"] != "PATRICE BERGERON") {
     print("FAILED: select test7\n");
  }
  if ($result["POSITION"] != "Forward") {
     print("FAILED: select test7\n");
  }
  if ($result["TEAM"] != "Bruins") {
     print("FAILED: select test7\n");
  }
  $stmt = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test8 - query and fetch with FETCH_OBJ
print("select test8\n");
$sql = "select * from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  $obj = $stmt->fetch(PDO::FETCH_OBJ);
  if ($obj->NUMBER != "37") {
     print("FAILED: select test8\n");
  }
  if ($obj->NAME != "PATRICE BERGERON") {
     print("FAILED: select test8\n");
  }
  if ($obj->POSITION != "Forward") {
     print("FAILED: select test8\n");
  }
  if ($obj->TEAM != "Bruins") {
     print("FAILED: select test8\n");
  }
  $stmt = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test9 - query and fetchObject
print("select test9\n");
$sql = "select * from Hockey_test where NUMBER=37";
class player {
  public $NUMBER;
  public $NAME;
  public $POSITION;
  public $TEAM;
}
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  $obj = $stmt->fetchObject('player');
  if ($obj->NUMBER != "37") {
     print("FAILED: select test9\n");
  }
  if ($obj->NAME != "PATRICE BERGERON") {
     print("FAILED: select test9\n");
  }
  if ($obj->POSITION != "Forward") {
     print("FAILED: select test9\n");
  }
  if ($obj->TEAM != "Bruins") {
     print("FAILED: select test9\n");
  }
  $obj = NULL;
  $stmt = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

// select test10 - failed query
print("select test10\n");
$sql = "select no_such_column from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $stmt = $db->query($sql);
  // TODO: should db->errorCode() be able to get an error code?
  if ($stmt) {
     print("FAILED: select test10\n");  
  }
} catch(PDOException $e) {  
  echo $e->getMessage();
  print("\n");  
}
$db = NULL;  

// select test11 - query and fetch with missing column.
print("select test11\n");
$sql = "select NAME from Hockey_test where NUMBER=37";
try {  
  $db = open_db();
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
  foreach ($db->query($sql) as $row)
  {
     if ($row["NAME"] != "PATRICE BERGERON") {
        print("FAILED: select test11 - NAME\n");
     }
     if (isset($row["POSITION"])) {
     	print("FAILED: select test11 - POSITION\n");
     }
  }
} catch(PDOException $e) {  
  echo $e->getMessage();  
}

drop_table_hockey_test();

echo "\ndone\n";
?>

--EXPECT--
drop table Hockey_test
create table Hockey_test
select test1
select test2
insert test1
select test3
delete test1
select test4
select test5
select test6
select test7
select test8
select test9
select test10
SQLSTATE[42703]: Undefined column: -54 can't resolve field "NO_SUCH_COLUMN"
SQL: select no_such_column from Hockey_test where NUMBER=37
select test11
drop table Hockey_test

done
