<?php /* $Id: testdb.inc 161049 2012-03-26 01:37:06Z tgates $ */

$user = 'cloud';
$password = 'user';
$db = NULL;  
ini_set('nuodb.default_user',$user);
ini_set('nuodb.default_password',$password);

/* we need just the generated name, not the file itself */
unlink($test_base = tempnam('/tmp',"php_nuodb_test"));

function open_db() 
{
  global $db, $user, $password;
  $db = new PDO("nuodb:database=test@localhost;schema=USER", $user, $password) or die;
  return $db;
}

function init_db()
{
  global $db;
	try {
	    $db = open_db();
	    $sql = "create table test1 (i integer, c varchar(100), ts timestamp, d date, t time)";
	    $db->query($sql);
	    $db->commit();
	    $sql = "insert into test1(i, c) values(1, 'test table not created with isql')";
	    $db->query($sql);
	    $db->commit();
	    $sql = "insert into test1(i, c, ts, d, t) values(2, 'test', 'today', '1962-12-28', '12:34:56')";
	    $db->query($sql);
	    $db->commit();

	} 
	catch(PDOException $e) {	
    	  echo "FAILED: ";
	  echo $e->getMessage(); 
	}
}

function cleanup_db()
{
  global $db;
	try {
	    $sql = "drop table test1";
	    $db->query($sql);
	} 
	catch(PDOException $e) {	
    	  echo "FAILED: ";
	  echo $e->getMessage(); 
	}
}

register_shutdown_function('cleanup_db');
init_db();
