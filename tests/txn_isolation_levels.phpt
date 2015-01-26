--TEST--
transaction isolation levels test
--FILE--
<?php 

require("testdb.inc");
global $db;
open_db();

/* NuoDB Isolation Levels are:
**
**      7 = ConsistentRead (Default) 
**      5 = WriteCommitted
**      2 = ReadCommitted
**      8 = Serializable
*/

try {

   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_CONSISTENT_READ) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_CONSISTENT_READ, got " . $txn_osi . "/n";
   }

   $db->setAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL, PDO::NUODB_TXN_ISOLATION_WRITE_COMMITTED);
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_WRITE_COMMITTED) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_WRITE_COMMITTED, got " . $txn_osi . "/n";
   }

   $db->setAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL, PDO::NUODB_TXN_ISOLATION_READ_COMMITTED);
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_READ_COMMITTED) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_READ_COMMITTED, got " . $txn_osi . "/n";
   }

   $db->setAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL, PDO::NUODB_TXN_ISOLATION_SERIALIZABLE);
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_SERIALIZABLE) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_SERIALIZABLE, got " . $txn_osi . "/n";
   }


   $db = NULL;
   ini_set("pdo_nuodb.default_txn_isolation", "ConsistentRead");
   open_db();
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_CONSISTENT_READ) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_CONSISTENT_READ, got " . $txn_osi . "/n";
   }

   $db = NULL;
   ini_set("pdo_nuodb.default_txn_isolation", "WriteCommitted");
   open_db();
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_WRITE_COMMITTED) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_WRITE_COMMITTED, got " . $txn_osi . "/n";
   }

   $db = NULL;
   ini_set("pdo_nuodb.default_txn_isolation", "ReadCommitted");
   open_db();
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_READ_COMMITTED) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_READ_COMMITTED, got " . $txn_osi . "/n";
   }

   $db = NULL;
   ini_set("pdo_nuodb.default_txn_isolation", "Serializable");
   open_db();
   $txn_iso = $db->getAttribute(PDO::NUODB_ATTR_TXN_ISOLATION_LEVEL);
   if ($txn_iso != PDO::NUODB_TXN_ISOLATION_SERIALIZABLE) {
      echo "Error, expected txn iso level PDO::NUODB_TXN_ISOLATION_SERIALIZABLE, got " . $txn_osi . "/n";
   }

} catch(PDOException $e) {
    echo $e->getMessage();
}

$db = NULL;
echo "done\n";
?>

--EXPECT--
done
