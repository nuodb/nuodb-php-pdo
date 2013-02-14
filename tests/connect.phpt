--TEST--
PDO_Nuodb: connect/disconnect
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php /* $Id: connect.phpt 161049 2012-03-26 01:37:06Z tgates $ */

  echo "Checking is pdo_nuodb extension is loaded: ";
  var_dump(extension_loaded('pdo_nuodb'));

  require("testdb.inc");

  global $db;
  $db = open_db();
	try {
	    $sql = "create table test1 (i integer, c varchar(100), ts timestamp, d date, t time)";
	    $db->query($sql);
	    $sql = "insert into test1(i, c) values(1, 'test table not created with nuosql')";
	    $db->query($sql);
	    $sql = "insert into test1(i, c, ts, d, t) values(2, 'test', 'today', '1962-12-28', '12:34:56')";
	    $db->query($sql);

	} 
	catch(PDOException $e) {	
    	  echo "FAILED: ";
	  echo $e->getMessage(); 
	}
   
	try {
	    $sql = "DROP TABLE test1 CASCADE IF EXISTS";
	    $db->query($sql);
	} 
	catch(PDOException $e) {	
    	  echo "FAILED: ";
	  echo $e->getMessage(); 
	}

  echo "done\n";
	
?>
--EXPECT--
Checking is pdo_nuodb extension is loaded: bool(true)
done

