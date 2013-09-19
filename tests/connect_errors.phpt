--TEST--
Attempt to connect, checking various error condition/processing/messages.
--FILE--
<?php 

$nuotestport = getenv("NUOTESTPORT");
if (!$nuotestport) {
  $nuotestport = "48004";
}

try {  
  $url = "nuodb:database=no-such-test@localhost:" . $nuotestport . ";schema=no-such-schema";
  $db = new PDO($url, "dba", "goalie", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'SQLSTATE[08000] [-7] no NuoDB nodes are available for database "no-such-test@localhost:' . $nuotestport . '"';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "no-such-user", "goalie", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'SQLSTATE[58000] [-13] "no-such-user" is not a known user for database "test"';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "dba", "wrong-password", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'SQLSTATE[58000] [-13] Authentication failed';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

try {  
  $url = "nuodb:database=test@no-such-localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "dba", "wrong-password", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = "SQLSTATE[08000] [-10] can't find broker for database" . ' "test@no-such-localhost:' . $nuotestport . '"';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

$db = NULL;  
echo "done\n";
?>
--EXPECT--
done


