--TEST--
Attempt to connect, checking various error condition/processing/messages.
--FILE--
<?php 

require("testdb.inc");


// First prove that we CAN connect.
echo "Test successful connection\n";
try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport;
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  echo "FAILED: connect failed when it should succeed.\n";
  $db = NULL;
}


echo "Test NULL url\n";
try {  
  $url = NULL;
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'invalid data source name';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

echo "Test with bad pdo name\n";
try {  
  $url = "no-such-driver-name";
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'invalid data source name';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

echo "Test with missing parameters\n";
try {  
  $url = "nuodb:";
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = "SQLSTATE[HY000] [-10] DSN is missing 'database' parameter. DSN=\"\"";
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}


echo "Test with dbname parameter\n";
try {  
  $url = "nuodb:dbname=test@localhost:" . $nuotestport;
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  // we can get a different exception message depending on how
  // the NuoDB processes were started.  All we need to do is 
  // is confirm that an exception message occurs.
  $expected_message = "SQLSTATE[HY000] [-10] DSN is missing 'database' parameter. DSN=\"dbname=";
  if (strncmp($expected_message, $caught_message, strlen($expected_message))) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

echo "Test with database=no-such-test@localhost\n";
try {  
  $url = "nuodb:database=no-such-test@localhost:" . $nuotestport . ";schema=no-such-schema";
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'SQLSTATE[08000] [-7] no NuoDB nodes are available for database "no-such-test@localhost:' . $nuotestport . '"';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}


echo "Test with missing username\n";
try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, NULL, "no-such-pass", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = "SQLSTATE[HY000] [-10] DSN is missing 'user' parameter";
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

echo "Test with non-existing username\n";
try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "no-such-user", "no-such-pass", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = 'SQLSTATE[58000] [-13] "no-such-user" is not a known user for database "test"';
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}

echo "Test with missing password\n";
try {  
  $url = "nuodb:database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "dba", NULL, array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = "SQLSTATE[HY000] [-10] DSN is missing 'password' parameter";
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}


echo "Test with wrong password\n";
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

echo "Test with database=test@no-such-localhost\n";
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

echo "Test with extra junk parameters\n";
try {  
  $url = "nuodb:foo=bar;database=test@localhost:" . $nuotestport . ";schema=USER";
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  echo "FAILED: " . $caught_message . "\n";
  $db = NULL;
}

echo "Test with junk parameters\n";
try {  
  $url = "nuodb:foo=bar;baz=bad";
  $db = new PDO($url, "dba", "dba", array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION)) or die;
  $db = NULL;
} catch(PDOException $e) {  
  $caught_message = $e->getMessage();
  $expected_message = "SQLSTATE[HY000] [-10] DSN is missing 'database' parameter. DSN=\"foo=bar;baz=bad\"";
  if (strcmp($expected_message, $caught_message)) {
     echo "FAILED: " . $caught_message . "\n";
  }
  $db = NULL;
}


$db = NULL;  
echo "done\n";
?>
--EXPECT--
Test successful connection
Test NULL url
Test with bad pdo name
Test with missing parameters
Test with dbname parameter
Test with database=no-such-test@localhost
Test with missing username
Test with non-existing username
Test with missing password
Test with wrong password
Test with database=test@no-such-localhost
Test with extra junk parameters
Test with junk parameters
done
