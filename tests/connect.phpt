--TEST--
PDO_Nuodb: connect/disconnect
--SKIPIF--
<?php include("skipif.inc"); ?>
--FILE--
<?php /* $Id: connect.phpt 161049 2012-03-26 01:37:06Z tgates $ */

	require("testdb.inc");
    
	echo "done\n";
	
?>
--EXPECT--
done
