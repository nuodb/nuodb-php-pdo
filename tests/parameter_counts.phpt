--TEST--
parameter_counts
--FILE--
<?php 
date_default_timezone_set('America/New_York');

require("testdb.inc");
global $db;  
open_db();

try {  
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  echo "Test case for insufficent number of parameters.\n";
  $sql1 = "SELECT :param1=:param2 AS TEST FROM DUAL";
  $stmt1 = $db->prepare($sql1);
  $param1 = 1;
  $param2 = 2;
  $stmt1->bindParam(':param1', $param1, PDO::PARAM_INT);
  $stmt1->execute();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "\ndone\n";
?>
--EXPECT--
Test case for insufficent number of parameters.
SQLSTATE[HY093]: Invalid parameter number: -12 number of bound variables does not match number of tokens
done
