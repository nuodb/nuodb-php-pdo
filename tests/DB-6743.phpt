--TEST--
DB-6743 -  PDO::ATTR_ERRMODE
--FILE--
<?php

    require("testdb.inc");// Load DB
    global $db;  
    open_db();

    //TEST FOR ATTR_ERRMODE valid Strategy modes
    $valid = array(PDO::ERRMODE_SILENT, PDO::ERRMODE_WARNING, PDO::ERRMODE_EXCEPTION); //get list of valid strats
    foreach($valid as $i){
        if(false == $db->setAttribute(PDO::ATTR_ERRMODE,$i))
            echo "FAILED: A valid ERRMODE was deemed impossible to set";
    }
    $sql = "A BAD SQL STRING"; //default invalid sql string to use 

    //TEST FOR ERRMODE_SILENT
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);
    $result = $db->query($sql);
    $code = $db->errorCode();
    $info = $db->errorInfo();

    if($result!=false)
        echo "FAILED: Expecting false for an invalid SQL string\n";
    if($code != "42000")
        echo "FAILED: Expecting SQL code 42000, received ", $code,"\n";
    if($code !== $info[0])
        echo "FAILED: errorCode and info[0] should be identical, received code = ", $code," expected info[0] = ",$info[0],"\n";
    if(empty($info[1]))
        echo "FAILED: No Driver specific error code received\n";
    elseif(-1==$info[1])
        echo "FAILED: Driver threw driver error code -1\n";
    if(empty($info[2]))
        echo "FAILED: No Driver specific error message received\n";

    //TEST FOR ERRMODE_WARNING <- similar to ERRMODE_SILENT; but emits E_WARNING
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
    function warning_handler($errno, $errmsg, $errfile, $errline){
        global $db,$line;
        echo "ERROR: Level [$errno] on line $errline\n";
        if($errno!=2)
            echo "FAILED: E_WARNING not emitted, expected errno = 2 received errno = ",$errno,"\n";
        $info = $db->errorInfo();
        if(false === stristr($errmsg, $info[2]))
            echo "FAILED: E_WARNING is not reporting the correct Driver errorMsg\n";
        
        if($errfile !== __FILE__)
            echo "FAILED: E_WARNING is not reporting the correct file; thrown in file ",__FILE__, " reported as in file ",$errfile,"\n";
        if($errline!=$line)
            echo "FAILED: E_WARNING is not reporting the correct line; thrown in line ",$line," reported as in line ",$errline,"\n";
    }
    set_error_handler('warning_handler');
    $line = __LINE__ +1;
    $db->query($sql);
    $code = $db->errorCode();
    $info = $db->errorInfo();

    if($code != "42000")
        echo "FAILED: Expecting SQL code 42000, received ", $code,"\n";
    if($code !== $info[0])
        echo "FAILED: errorCode and info[0] should be identical, received code = ", $code," expected info[0]",$info[0],"\n";
    if(empty($info[1]))
        echo "FAILED: No Driver specific error code received\n";
    elseif(-1==$info[1])
        echo "FAILED: Driver threw driver error code -1\n";
    if(empty($info[2]))
        echo "FAILED: No Driver specific error message received\n";
    restore_error_handler();    

    //TEST FOR ERRMODE_EXCEPTION <- Throws PDOException
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    try{
        $line = __LINE__ +1;
        $db->query($sql);
    } catch (PDOException $e) {
        echo "EXCEPTION: PDOException should be thrown by ERRMODE_EXCEPTION\n";
        $code = $db->errorCode();
        $info = $db->errorInfo();
        
        if($code != "42000")
            echo "FAILED: Expecting SQL code 42000, received ", $code,"\n";
        if($code !== $info[0])
            echo "FAILED: errorCode and info[0] should be identical, received code = ", $code," expected info[0]",$info[0],"\n";
        if(empty($info[1]))
            echo "FAILED: No Driver specific error code received\n";
        elseif(-1==$info[1])
            echo "FAILED: Driver threw driver error code -1\n";
        if(empty($info[2]))
            echo "FAILED: No Driver specific error message received\n";
        if($e->getCode() !==$code)
            echo "FAILED: PDOException errorcode does not match recieved errorCode";
        if(false === stristr($e->getMessage(), $info[2]))
            echo "FAILED: PDOException is not reporting the correct Driver errorMsg\n";
        if($e->getFile() !== __FILE__)
            echo "FAILED: PDOException is not reporting the correct file; thrown in file ",__FILE__, " reported as in file ",$e->getFile(),"\n";
        if($e->getLine()!=$line)
            echo "FAILED: PDOException is not reporting the correct line; thrown in line ",$line," reported as in line ",$e->getLine(),"\n";
    }

    $db = null;
    echo 'done';
?>
--EXPECT--
FAILED: Driver threw driver error code -1
ERROR: Level [2] on line 52
FAILED: Driver threw driver error code -1
EXCEPTION: PDOException should be thrown by ERRMODE_EXCEPTION
FAILED: Driver threw driver error code -1
done
