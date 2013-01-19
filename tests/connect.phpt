--TEST--
PDO_Nuodb: connect/disconnect
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php /* $Id: connect.phpt 161049 2012-03-26 01:37:06Z tgates $ */

  echo "Checking is pdo_nuodb extension is loaded: ";
  var_dump(extension_loaded('pdo_nuodb'));

  require("testdb.inc");
   
  echo "done\n";
	
?>
--EXPECT--
Checking is pdo_nuodb extension is loaded: bool(true)
done

