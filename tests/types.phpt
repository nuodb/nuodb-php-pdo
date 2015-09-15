--TEST--
NuoDB PDO->exec(), testing of native types - stringify off
--FILE--
<?php
require("testdb.inc");// Load DB
global $db;
open_db();

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
$db->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, false);

function tests($db){
    $is_mysqlnd = true;
    
    // ***BOOLEAN TYPES***
    
    test_case($db, 10, 'BOOLEAN', 1, true); // Similar to a bit(1)
    test_case($db, 11, 'BOOLEAN', 0, false);
    test_case($db, 12, 'BOOLEAN', true, true);
    //test_case($db, 13, 'BOOLEAN', false, false); // @TODO DB-9736
    test_case($db, 14, 'BOOLEAN', "True", true);
    test_case($db, 15, 'BOOLEAN', "False", false);
     
    // ***NUMERIC TYPES***
    // Gasp! We lost our 20s!
    
    test_case($db, 30, 'SMALLINT', -32768, -32768);
    test_case($db, 31, 'SMALLINT', 32767, 32767);
    test_case($db, 32, 'SMALLINT', ".5", 1); // NuoDB rounds up from .5
    test_case($db, 33, 'SMALLINT', ".499999999", 0); // NuoDB rounds up from .5
    
    test_case($db, 40, 'INTEGER', (int)-2147483648, (int)-2147483648);
    test_case($db, 41, 'INTEGER', 2147483647, 2147483647);
    test_case($db, 42, 'INTEGER', ".5", 1); // NuoDB rounds up from .5
    test_case($db, 43, 'INTEGER', ".499999999", 0); // NuoDB rounds up from .5
    
    test_case($db, 50, 'INT', (int)-2147483648, (int)-2147483648);
    test_case($db, 51, 'INT', 2147483647, 2147483647);
    test_case($db, 52, 'INT', ".5", 1); // NuoDB rounds up from .5
    test_case($db, 53, 'INT', ".499999999", 0); // NuoDB rounds up from .5
    
    //@TODO should give some thought into int64 vs int32 capabilities
    //test_case($db, 100, 'INT', ((PHP_INT_SIZE > 4) ? (int)-9223372036854775808 : (int) -2147483648), ((PHP_INT_SIZE > 4) ? (int)-9223372036854775808 : -2147483648));
    // php will interpret out of bounds type as float/double
    //test_case($db, 100, 'INT', -2147483648, (PHP_INT_SIZE > 4) ? (int)-2147483648 : (double)-2147483648);
    
    // Current Driver Implemenation returns BIGINT as PDO STRING
    test_case($db, 60, 'BIGINT', '1', '1');
    test_case($db, 61, 'BIGINT', '-9223372036854775808', '-9223372036854775808');
    test_case($db, 62, 'BIGINT', '9223372036854775807', '9223372036854775807');
    test_case($db, 63, 'BIGINT', '.5', '1'); // NuoDB rounds up from .5
    test_case($db, 64, 'BIGINT', '.499999999', '0'); // NuoDB rounds up from .5
    
    // ***NUMERICS W/PRECISION*** -- Current Driver Implementation returns as PDO STRING
    
    // Note Decimals and Nemerics without precision and scale are treated like Number
    test_case($db, 70, 'DECIMAL', '-32768', '-32768');
    test_case($db, 71, 'DECIMAL', '32767', '32767');
    test_case($db, 72, 'DECIMAL(6,4)', '12.3456', '12.3456');
    test_case($db, 73, 'DECIMAL(6,4)', '12.34567', '12.34567', $fail = true); // Expect fail
    
    test_case($db, 80, 'NUMERIC', '-32768', '-32768');
    test_case($db, 81, 'NUMERIC(4)', '32767', '32767');
    test_case($db, 83, 'NUMERIC(20)', '-2.135987e+96',
        '-2135987000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'
    ); // should utilize regex for this!
    test_case($db, 84, 'NUMERIC(20)', '-2.135987e+100',
        '-21359870000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000',
        $fail = false // THIS SHOULD FAIL! Leaving as 'expecting no fail' for time being
    ); // @TODO Investigate why this doesn't fail, I suspect an overflow will force nuodb to treat type as Number
    
    test_case($db, 90, 'NUMBER', 
        $inserted_value = '1.12345678901234567890123456789012345678901', 
        $expected_value = '1.12345678901234567890123456789012345678901'
    ); // match 42 precision & 41 scale
    
    test_case($db, 100, 'DOUBLE', '-1.797693134862315e+308', '-1.797693134862315e+308'); //about min
    test_case($db, 101, 'DOUBLE', '-1.797693134862315e+309', '-inf'); // unsupported min
    test_case($db, 102, 'DOUBLE', '1.797693134862315e+308', '1.797693134862315e+308'); //about min
    test_case($db, 103, 'DOUBLE', '1.797693134862315E+309', 'inf'); // unsupported max
    
    test_case($db, 110, 'FLOAT', '-1.797693134862315e+308', '-1.797693134862315e+308'); //about min
    test_case($db, 111, 'FLOAT', '-1.797693134862315e+309', '-inf'); // unsupported min
    test_case($db, 112, 'FLOAT', '1.797693134862315e+308', '1.797693134862315e+308'); //about min
    test_case($db, 113, 'FLOAT', '1.797693134862315E+309', 'inf'); // unsupported max
    
    // ***DATE & TIME***
    
    //@TODO increase (scope) testing of these types
// TODO: Uncomment the next two test cases when DB-9740 is fixed.
//    test_case($db, 120, 'DATE', "2008-04-23", 1208923200); // @TODO DB-9740: An integer cast reveals the value should be 1208908800
//    test_case($db, 130, 'TIME', '14:37:00',70620);
    test_case($db, 140, 'TIMESTAMP', '2008-05-06 21:09:00', "2008-05-06 21:09:00");
    test_case($db, 150, 'DATETIME', '2008-03-23 14:38:00', "2008-03-23 14:38:00");
    
    // ***CHARS & STRINGS***
    test_case($db, 160, 'CHAR(1)', 'a', 'a');
    test_case($db, 161, 'CHAR(1)', 'ab', NULL, $fail = true); // Failure leading statement
    test_case($db, 162, 'CHAR', 'a', 'a');
    test_case($db, 163, 'CHAR', 'ab', NULL, $fail = true); // Failure leading statement
    test_case($db, 164, 'CHAR(1000)', str_repeat('z', 1000), str_repeat('z', 1000)); // NuoDB, as of this writing, is unlimited on chars and string types

    test_case($db, 170, 'VARCHAR(1)', 'a', 'a');
    test_case($db, 171, 'VARCHAR(1)', 'ab', NULL, $fail = true); // Failure leading statement
    test_case($db, 172, 'VARCHAR(1000)', str_repeat('z', 1000), str_repeat('z', 1000)); // NuoDB, as of this writing, is unlimited on chars and string types
    
    test_case($db, 180, 'character varying(1)', 'a', 'a');
    test_case($db, 181, 'character varying(1)', 'ab', NULL, $fail = true); // Failure leading statement
//     test_case($db, 182, 'character varying', 'a', 'a'); // @TODO DB-9741: Invalid SQL statement, but this seems to cause a Connection loss???
    test_case($db, 183, 'character varying(1000)', str_repeat('z', 1000), str_repeat('z', 1000)); // NuoDB, as of this writing, is unlimited on chars and string types
    
    test_case($db, 190, 'character(1)', 'a', 'a');
    test_case($db, 191, 'character(1)', 'ab', NULL, $fail = true); // Failure leading statement
    test_case($db, 192, 'character', 'a', 'a');
    test_case($db, 193, 'character', 'ab', NULL, $fail = true); // Failure leading statement
    test_case($db, 194, 'character(1000)', str_repeat('z', 1000), str_repeat('z', 1000)); // NuoDB, as of this writing, is unlimited on chars and string types
    
    test_case($db, 200, 'char large object (1)', 'a', 'a'); // is 'char large object (n)' the same as 'char large object' ?
    
    test_case($db, 210, 'clob (1)', 'a', 'a'); // is 'clob (n)' the same as 'clob' ?
    
    test_case($db, 220, 'nchar large object', 'a', 'a');
    test_case($db, 221, 'nchar large object', str_repeat('b', 255), str_repeat('b', 255));

    test_case($db, 230, 'text', 'a', 'a');
    test_case($db, 231, 'text', str_repeat('b', 255), str_repeat('b', 255));
    
    test_case($db, 240, 'string', 'a', 'a');
    test_case($db, 241, 'string', str_repeat('b', 255), str_repeat('b', 255));    
}

tests($db);

echo "done!\n";

function test_case(&$db, $id, $sql_type, $value, $expect, $fail = false) {
    
    $db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, false);

    $db->exec('DROP TABLE IF EXISTS test_types');
    $sql = sprintf('CREATE TABLE test_types(id INT, value %s)', $sql_type);
    try {
        @$db->exec($sql);
    } catch (Exception $e) {
        // Server version might not support that type
        printf("-- %s -- CREATE TABLE failed: %s\n",$sql_type,$e->getMessage());
        return true; 
    }

    $stmt = $db->prepare('INSERT INTO test_types(id, value) VALUES (?, ?)');
    $stmt->bindValue(1, $id);
    $stmt->bindValue(2, $value);
    try {
        if (!$stmt->execute()) {
            if (!$fail) {
                printf("-- %s::%d -- INSERT failed: %s\n", $sql_type, $id, var_export($stmt->errorInfo(), true));
                return false;
            } else {
                return true;
            }
        }
    } catch (Exception $e) {
        if (!$fail){
            printf("-- %s::%d -- EXECUTE failed: %s\n", $sql_type, $id, var_export($e->getMessage(), true));
            return false;
        } else {
            return true;
        }
    }
    $db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, false);
    $stmt = $db->query('SELECT  id, value FROM test_types');
    $row = $stmt->fetch(PDO::FETCH_ASSOC);
    $stmt->closeCursor();

    if (!isset($row['ID']) || !isset($row['VALUE'])) {
        if (!$fail){
            printf("-- %s::%d -- Fetched result is empty, dumping result: %s\n", $sql_type, $id, var_export($row, true));
            return false;
        } else {
            return true;
        }
    }

    if ($row['ID'] != $id) {
        if (!$fail){
            printf("-- %s -- Invalid ID: Expecting %s got %s\n", $sql_type, $id, $row['ID']);
            return false;
        } else {
            return true;
        }     
    }

    if ($row['VALUE'] != $expect) {
        if (!$fail) {
            printf("-- %s::%d -- Invalid VALUE: Expecting %s got %s \n", $sql_type, $id, $expect, $row['VALUE']);
            return false;
        } else {
            return true;
        }
    }
    
    if ($row['VALUE'] !== $expect) {
        if (!$fail) {
            printf("-- %s::%d -- Invalid VALUE type: Expecting %s got %s \n", $sql_type, $id, strtoupper(gettype($expect)), strtoupper(gettype($row['VALUE'])));
            return false;
        } else {
            return true;
        }
    }

    $db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, true);
    $stmt = $db->query('SELECT id, value FROM test_types');
    $row_string = $stmt->fetch(PDO::FETCH_ASSOC);
    $stmt->closeCursor();
    if ($row['VALUE'] != $row_string['VALUE']) {
        if (!$fail) {
            printf("-- %s::%d -- STRINGIGY = %s, NATIVE = %s\n", $sql_type, $id, var_export($row_string['VALUE'], true), var_export($row['VALUE'], true));
            return false;
        } else {
            return true;
        }
    } 
    
    return true;
}
?>
--EXPECTF--
done!
