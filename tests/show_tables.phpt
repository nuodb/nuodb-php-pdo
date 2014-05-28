--FILE--
Show Table test.
--FILE--
<?php

require("testdb.inc");
global $db;
open_db();

try {

  $sql1 = "drop table test cascade if exists";
  $db->query($sql1);

  $sql2 = "create table test (a char)";
  $db->query($sql2);

  $sql3 = "SHOW TABLES";
  $db->query($sql3);

  $sql4 = "drop table test cascade if exists";
  $db->query($sql4);
 
  $db = NULL;

} catch(PDOException $e) {
  echo $e->getMessage();
}

echo "done\n";

?>
--EXPECT--
done
