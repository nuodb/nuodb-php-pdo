--TEST--
client-server version test
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

  try {
        $server_version = $db->getAttribute(PDO::ATTR_SERVER_VERSION);
        $client_version  = $db->getAttribute(PDO::ATTR_CLIENT_VERSION);

        if ('' == $server_version) {
                echo "Server version must not be empty\n";
	}
	
	if ('' == $client_version) {
                echo "Client version must not be empty\n";
	}

        if (is_string($server_version)) {
               if (!preg_match('/(\d+)\.(\d+)\.(\d+)\-(.*)/', $server_version, $matches)) {
                       printf("[001] Server version string seems wrong, got '%s'\n", $server_version);
               }
	       else {
		        echo "NuoDB server version is $matches[0]\n"; 	

 			/* The following conditions can be used if required in case of
    			   checking the Major version,sub version or fix pack version */		
  
  //                    if ($matches[1] < 2)
  //                           printf("[002] This is not the latest major version: '%s'. can be upgraded\n", $matches[1]);
  //                    if ($matches[2] > 0)
  //                          printf("[003] , This is server fix pack version -'%s'\n", $matches[2]);
  //                    if ($matches[3])
  //                          printf("[004] This is server sub version - '%s'\n", $matches[2]);
	      }
	}
	else {
		echo "NuoDB server version is $server_version\n";
	}

	if (is_string($client_version)) {
                if (!preg_match('/(\d+)\.(\d+)\.(.*)/', $client_version, $matches)) {
                        printf("[001] Client version string seems wrong, got '%s'\n", $client_version);
                }
                else {    
                        echo "NuoDB client version is $matches[0]\n";
		
		    	   /* The following conditions can be used if required in case of
    			   checking the Major version,sub version or fix pack version */

  //                    if ($matches[1] < 2)
  //                           printf("[002] This is not the latest major version: '%s'. can be upgraded\n", $matches[1]);
  //                    if ($matches[2] > 0)
  //                           printf("[003] , This is client fix pack version '%s'\n", $matches[2]);
  //                    if ($matches[3])
  //                           printf("[004] this is client sub version - '%s'\n", $matches[2]);

		}
	}
	else {
		echo "NuoDB client version is $client_version\n";
	}	

} catch(PDOException $e) {
  echo $e->getMessage();
}

$db = NULL;
echo "done\n";
?>
--EXPECT--
NuoDB server version is 2.0.3-19
NuoDB client version is 2.0.4
done
