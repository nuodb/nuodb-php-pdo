--TEST--
Attempt to connect, checking various error condition/processing/messages.
--FILE--
<?php 
try {  
  $db = new PDO("nuodb:database=no-such-test@localhost;schema=no-such-schema", "dba", "goalie") or die;
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
  $db = NULL;
}

try {  
  $db = new PDO("nuodb:database=test@localhost;schema=USER", "no-such-user", "goalie") or die;
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
  $db = NULL;
}

try {  
  $db = new PDO("nuodb:database=test@localhost;schema=USER", "dba", "wrong-password") or die;
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
  $db = NULL;
}

try {  
  $db = new PDO("nuodb:database=test@no-such-host;schema=USER", "dba", "wrong-password") or die;
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage() . "\n";  
  $db = NULL;
}

$db = NULL;  
echo "done\n";
?>
--EXPECT--
SQLSTATE[HY000] [38] no NuoDB nodes are available for database "no-such-test@localhost"
SQLSTATE[HY000] [38] "no-such-user" is not a known user for database "test"
SQLSTATE[HY000] [38] Authentication failed
SQLSTATE[HY000] [38] can't find broker for database "test@no-such-host"
done


