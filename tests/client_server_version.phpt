--TEST--
client-server version test
--FILE--
<?php 

require("testdb.inc");
global $db;
open_db();

try {
    //Release format follows: X.Y[.P][.C][-B][-S]
    $server_version = $db->getAttribute(PDO::ATTR_SERVER_VERSION);
    $client_version  = $db->getAttribute(PDO::ATTR_CLIENT_VERSION);

    if ('' == $server_version) {
        echo "Server version must not be empty\n";
    }

    if ('' == $client_version) {
        echo "Client version must not be empty\n";
    }

    if (is_string($server_version)) {
        if (!preg_match('/(\d+)\.(\d+)(\.\d+)?(\.[A-Za-z0-9]+)?(\-\d+)?(\-[a-z0-9]+)?/', $server_version, $matches)) {
            printf("[001] Server version string seems wrong, got '%s'\n", $server_version);
            // TODO compare Major.Minor.Patch numbers against current latest version
            // So the version numbers should not be greater than the current latest.
        }
        else {
            echo "NuoDB server version is valid\n";
        }
    }
    else {
        echo "NuoDB server version is valid\n";
    }

    if (is_string($client_version)) {
        if (!preg_match('/(\d+)\.(\d+)(\.\d+)?(\.[A-Za-z0-9]+)?(\-\d+)?(\-[a-z0-9]+)?/', $client_version, $matches)) {
            printf("[001] Client version string seems wrong, got '%s'\n", $client_version);
        }
        else {
            echo "NuoDB client version is valid\n";
        }
    }
    else {
        echo "NuoDB client version is valid\n";
    }

} catch(PDOException $e) {
    echo $e->getMessage();
}

$db = NULL;
echo "done\n";
?>

--EXPECT--
NuoDB server version is valid
NuoDB client version is valid
done
