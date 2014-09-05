--TEST--
Connect to DB with a list of Brokers
--SKIPIF--
<?php
    require("testdb.inc");
    $db = open_db();
    $version = $db->getAttribute(PDO::ATTR_SERVER_VERSION);
    if (preg_match('/^NuoDB\s\d\.\d/', $version, $matches)) {
        $version_no = (substr($matches[0], stripos($matches[0], ' ') + 1, 3) * 10);
        if( $version_no < 21 ) {
            die(sprintf('skip NuoDB version "%s" does not support feature', $version));
        }
    } else {
        die(sprintf('skip improper NuoDB version found - "%s"',$version));
    }
    unset($db);
?>
--FILE--
<?php
    require("testdb.inc");
    global $db, $user, $password;
    
    #ASSUMPTIONS: Main broker with TE/SM is currently running on port 48004
    # or any port specified by the NUOTESTPORT
    $deadPort = "9";
    $main_broker = "test@127.0.0.1:$nuotestport";
    $dead_broker = "test@127.0.0.1:$deadPort"; //this could be any port; represents non-running broker
    $schema = "schema=PDOTEST";

    $dsn = $main_broker; //verify main broker can be reached
    verifyEnv($dsn);
    
    $dsn = $dead_broker; //verify there is no broker at this location
    verifyEnv($dsn);
    
    $dsn = ""; //verify with empty broker string
    verifyEnv($dsn);
    
    $dsn = ","; //verify with empty multi-broker string
    verifyEnv($dsn);

    $dsn = ",,"; //verify with empty multi-broker string
    verifyEnv($dsn);

    $dsn = "$dead_broker,,$main_broker,"; //verify with empty & valid multi-broker string
    verifyEnv($dsn);

    $dsn = "$dead_broker,$dead_broker,$dead_broker"; //verify that we can send a multibroker string - all dead
    verifyEnv($dsn);
    
    $dsn = "$main_broker,$dead_broker,$dead_broker"; //verify that we can send a multibroker string - first will work
    verifyEnv($dsn);
    
    $dsn = "$dead_broker,$main_broker,$dead_broker"; //verify that the driver will choose the active broker
    verifyEnv($dsn);
    
    $dsn = "$dead_broker,$dead_broker,$dead_broker,$main_broker,$dead_broker"; //try many csv brokers
    verifyEnv($dsn);

    function verifyEnv($dsn, $MAX_ITER=1){
        try{
            $db = connectDB($dsn);
            if ($db === false) {
                throw new InvalidArgumentException("DB variable is not set");
            }
            for ( $i = 1; $i <=$MAX_ITER; $i++ ){
                $sql = "SELECT $i FROM DUAL";
                query($db,$sql);   
            }
        } catch(InvalidArgumentException $e){
            echo "FAILED: ";
            echo $e->getMessage(),"\n";
        }
        unset($db);
    }   
    
    //====Helper script functions
    function connectDB($dsn){
        global $user, $password, $schema;
        print "\n==== Connecting...\n";
        $url = "nuodb:database=$dsn;$schema";
        print "URL CONNECTION: $url\n";
        try {
            $db = new PDO($url, $user, $password) or die("Could not connect");
            $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        }
        catch(PDOException $e) {
            echo "FAILED: Error code ",$e->getCode(),"\n";
            return false;
        }
        print "CONNECTED\n";
        return $db;
    }
    
    function query($db,$sql){
        print "\n==== Querying DB\n";
        print "-- SQL: $sql\n";
        try{                
            $result = $db->query($sql)->fetchAll();
            foreach ($result as $row) {
                print_r($row);
            }
        }catch(Exception $e) {
            echo "FAILED: ";
            echo $e->getMessage(),"\n";
        }
    }           
?>
--EXPECTF--

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s;schema=PDOTEST
CONNECTED

==== Querying DB
-- SQL: SELECT 1 FROM DUAL
Array
(
    [] => 1
    [0] => 1
)

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s;schema=PDOTEST
FAILED: Error code -%d
FAILED: DB variable is not set

==== Connecting...
URL CONNECTION: nuodb:database=;schema=PDOTEST
FAILED: Error code -%d
FAILED: DB variable is not set

==== Connecting...
URL CONNECTION: nuodb:database=,;schema=PDOTEST
FAILED: Error code -%d
FAILED: DB variable is not set

==== Connecting...
URL CONNECTION: nuodb:database=,,;schema=PDOTEST
FAILED: Error code -%d
FAILED: DB variable is not set

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s,,test@127.0.0.1:%s,;schema=PDOTEST
CONNECTED

==== Querying DB
-- SQL: SELECT 1 FROM DUAL
Array
(
    [] => 1
    [0] => 1
)

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s;schema=PDOTEST
FAILED: Error code -%d
FAILED: DB variable is not set

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s;schema=PDOTEST
CONNECTED

==== Querying DB
-- SQL: SELECT 1 FROM DUAL
Array
(
    [] => 1
    [0] => 1
)

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s;schema=PDOTEST
CONNECTED

==== Querying DB
-- SQL: SELECT 1 FROM DUAL
Array
(
    [] => 1
    [0] => 1
)

==== Connecting...
URL CONNECTION: nuodb:database=test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s,test@127.0.0.1:%s;schema=PDOTEST
CONNECTED

==== Querying DB
-- SQL: SELECT 1 FROM DUAL
Array
(
    [] => 1
    [0] => 1
)

