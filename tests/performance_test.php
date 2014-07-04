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

  $start_time = microtime(true); 
  $db->beginTransaction();

  while ($i < $SMALL_ITERATION) {
         $sql = "INSERT INTO perf_test (a,b ) VALUES ($i,'A')";
         $i++;
         $db->query($sql);
  }
  $db->commit();
  $end_time = microtime(true);
   
  $elapse_time1 = $end_time - $start_time;
  echo "\n Elapse time of 100 Insert   =   $elapse_time1";

  $start_time = microtime(true);
  $sql4 = "select * from perf_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();

  while ($result = $stmt->fetch());
  
  $end_time = microtime(true);

  $elapse_time2 = $end_time - $start_time;
  echo "\n Elapse time of 100 Select   =   $elapse_time2";

  $sql1 = "drop table perf_test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table perf_test (a int,b char)";
  $db->query($sql2);

  $LARGE_ITERATION =  ($SMALL_ITERATION * 1000);
  $i = 0;


  $start_time = microtime(true);
  $db->beginTransaction();
 
  while ($i < $LARGE_ITERATION) {
    $sql = "INSERT INTO perf_test (a,b ) VALUES ($i,'A')";
    $i++;
    $db->query($sql);
  }
 
  $db->commit();

  $end_time = microtime(true);

  $elapse_time3 = $end_time - $start_time;
  echo "\n Elapse time of 100000 Insert =  $elapse_time3";

  $start_time = microtime(true);
  $sql4 = "select * from perf_test";
  $stmt = $db->prepare($sql4);
  $stmt->execute();

  while ($result = $stmt->fetch());

  $end_time = microtime(true);

  $elapse_time4 = $end_time - $start_time;
  echo "\n Elapse time of 100000 Select =  $elapse_time4";

  if( $elapse_time4 > ($elapse_time2 * 2000) ||  $elapse_time3 > ($elapse_time1 * 2000) )
  {
    	echo "\n TEST FAIL";
	exit(1);
  }
  else
  {
  	echo "\n TEST PASS";
	exit(0);
  }

	
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
?>
