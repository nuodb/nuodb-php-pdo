--TEST--
DB-4513 -- parameter reuse in prepared statements	
--FILE--
<?php 
date_default_timezone_set('America/New_York');

require("testdb.inc");
global $db;  
open_db();

try {  
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  $sql0 = 'DROP TABLE addresses CASCADE IF EXISTS';
  $db->query($sql0);

  $sql1 = 'create table addresses (addr1 string, addr2 string, shipping1 string, shipping2 string, phone string not null)';
  $db->query($sql1);

  $sql2 = "insert into addresses (addr1, addr2, shipping1, shipping2, phone) values (:address1, :address2, :address1, :address2, :phone)";
  $stmt2 = $db->prepare($sql2);
  $stmt2->execute(array(':address1'=>'215 First St', ':address2'=>'Cambridge, MA 02142', ':phone'=>'6175000001'));

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "\ndone\n";
?>
--EXPECT--
SQLSTATE[HY093]: Invalid parameter number: -12 number of bound variables does not match number of tokens
done