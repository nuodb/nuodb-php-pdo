--TEST--
Prepared Statements and BLOBs
--FILE--
<?php
	$db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;

	$blobs = array(
//		'TINYBLOB'		=> 255,
//		'TINYTEXT'		=> 255,
		'BLOB'				=> 32767,
//		'TEXT'				=> 32767,
//		'MEDIUMBLOB'	=> 100000,
//		'MEDIUMTEXT'	=> 100000,
//		'LONGBLOB'		=> 100000,
//		'LONGTEXT'		=> 100000,
	);

	function test_blob($db, $offset, $sql_type, $test_len) {

		$db->exec('DROP TABLE IF EXISTS CASCADE test');
		$db->exec(sprintf('CREATE TABLE test(id INT, label %s)', $sql_type));

		$value = null;
		$stmt = $db->prepare('INSERT INTO test(id, label) VALUES (:id, :label)');
		$id = 1;
		$stmt->bindParam(':id', $id, PDO::PARAM_INT);
		$stmt->bindParam(':label', $value, PDO::PARAM_LOB);
		if (!$stmt->execute()) {
			printf("[%03d + 1] %d %s\n",
				$offset, $stmt->errorCode(), var_export($stmt->errorInfo(), true));
			return false;
		}

		$stmt = $db->query('SELECT id, label FROM test');
		$id = $label = NULL;
		$stmt->bindColumn(1, $id, PDO::PARAM_INT);
		$stmt->bindColumn(2, $label, PDO::PARAM_LOB);

		if (!$stmt->fetch(PDO::FETCH_BOUND)) {
			printf("[%03d + 2] %d %s\n",
				$offset, $stmt->errorCode(), var_export($stmt->errorInfo(), true));
			return false;
		}

		if (strcmp($label, $value) !== 0) {
			printf("[%03d + 3] Returned value seems to be wrong (%d vs. %d charachters). Check manually\n",
				$offset, strlen($label), strlen($value));
			return false;
		}

		if (1 != $id) {
			printf("[%03d + 3] Returned id column value seems wrong, expecting 1 got %s.\n",
				$offset, var_export($id, true));
			return false;
		}

		return true;
	}

	$offset = 0;
	foreach ($blobs as $sql_type => $test_len) {
		test_blob($db, ++$offset, $sql_type, $test_len);
	}

	print "done!";
?>
--CLEAN--
<?php
$db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
$db->exec('DROP TABLE IF EXISTS CASCADE test');
?>
--EXPECTF--
done!