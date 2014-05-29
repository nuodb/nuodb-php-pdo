--FILE--
Show Table test.
--FILE--
<?php

require("testdb.inc");
global $db;
open_db();

try {

  $sql1 = "DROP TABLE test CASCADE IF EXISTS";
  $db->query($sql1);

  $sql2 = "CREATE TABLE test (C1 CHAR(10),C2 INTEGER)";
  $db->query($sql2);

  $sql3 = "SHOW TABLE test";

  foreach ($db->query($sql3) as $row) {
		print_r ($row[0]);
  }

  $sql4 = "DROP TABLE test CASCADE IF EXISTS";
  $db->query($sql4);
  $db = NULL;

} catch(PDOException $e) {
  echo $e->getMessage();
}

echo "\ndone\n";
?>
--EXPECT--

    Tables named TEST

    Found table TEST in schema PDOTEST

        Fields:
            C1 char(10)
            C2 integer
done
