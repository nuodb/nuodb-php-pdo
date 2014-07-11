--TEST--
Connect to DB with a list of Brokers
Should verify that this test is on Unix; Skip otherwise
--FILE--
<?php
require("testdb.inc");
global $db;

$nuotestport = getenv("NUOTESTPORT");
    if (! $nuotestport) {
        $nuotestport = "48004";
}

//create/use the intial url - assuming an instance of nuodb already running
$url = "nuodb:database=test@127.0.1.1:$nuotestport";


//build an array of brokers
$brokers = array();

//create a loop to build a list; for now test with 1
var_dump(addBroker());
echo "\n";

//pass in multi-broker string to create the PDO


//Required script function
function addBroker(){
    $output = array();
    $rv = -1;
    exec('ls', $output,$rv);
    
    return $output;
}
?>
--EXPECTF--


