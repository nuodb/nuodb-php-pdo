--TEST--
Perfromance test for SELECT statement.
--FILE--
<?php

require("testdb.inc");
global $db;
open_db();

try {  

  $sql1 = "drop table perf_test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table perf_test (a int,b char)";
  $db->query($sql2);

  $SMALL_ITERATION = 100;
  $i = 0;

  $db->beginTransaction();
  while ($i < $SMALL_ITERATION) {
         $sql = "INSERT INTO perf_test (a,b ) VALUES ($i,'A')";
         $i++;
         $db->query($sql);
 }
 $db->commit();

   
  $start_time = microtime(true);
  $sql4 = "select * from perf_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();

  $end_time = microtime(true);

  $elapse_time1 = $end_time - $start_time;
 // echo $elapse_time1;
 // echo "\n"; 


  $sql1 = "drop table perf_test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table perf_test (a int,b char)";
  $db->query($sql2);

  $LARGE_ITERATION =  ($SMALL_ITERATION * 1000);
  $i = 0;

  $db->beginTransaction();
 
  while ($i < $LARGE_ITERATION) {
    $sql = "INSERT INTO perf_test (a,b ) VALUES ($i,'A')";
    $i++;
    $db->query($sql);
  }
 
  $db->commit();

  $start_time = microtime(true);
  $sql4 = "select * from perf_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();

  $end_time = microtime(true);

  $elapse_time2 = $end_time - $start_time;
//  echo $elapse_time2;

  if( $elapse_time2 > ($elapse_time1 * 2000) )
	echo "TEST FAIL";
  else
        echo "TEST PASS";
	
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  

?>
--EXPECT--
TEST PASS
