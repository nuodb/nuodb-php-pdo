--TEST--
Prepared Statements and BLOBs
--FILE--
<?php
	$db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;

	$blobs = array(
//		'TINYBLOB'		=> 255,
//		'TINYTEXT'		=> 255,
		'BLOB'				=> 4,
//		'TEXT'				=> 4,
//		'MEDIUMBLOB'	=> 100000,
//		'MEDIUMTEXT'	=> 100000,
//		'LONGBLOB'		=> 100000,
//		'LONGTEXT'		=> 100000,
	);

	function test_blob($db, $offset, $sql_type, $test_len) {

		$db->exec('DROP TABLE IF EXISTS CASCADE test');
		$db->exec(sprintf('CREATE TABLE test(id INT, label %s)', $sql_type));

		$value = str_repeat('a', $test_len);
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
		$ret = $stmt->fetch(PDO::FETCH_ASSOC);

		if ($ret['LABEL'] !== $value) {
			printf("[%03d + 3] Returned value seems to be wrong (%d vs. %d charachters). Check manually\n",
				$offset, strlen($ret['label']), strlen($value));
			return false;
		}

		if (1 != $ret['ID']) {
			printf("[%03d + 3] Returned id column value seems wrong, expecting 1 got %s.\n",
				$offset, var_export($ret['id'], true));
			return false;
		}

		return true;
	}

	$offset = 0;
	foreach ($blobs as $sql_type => $test_len) {
//		$db->setAttribute(PDO::ATTR_EMULATE_PREPARES, 1);
		test_blob($db, ++$offset, $sql_type, $test_len);
//		$db->setAttribute(PDO::ATTR_EMULATE_PREPARES, 0);
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