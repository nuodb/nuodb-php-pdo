--TEST--
URI based connection test
--FILE--

<?php

require("testdb.inc");

  function getTempDir() {

       if (!function_exists('sys_get_temp_dir')) {
          
              if (!empty($_ENV['TMP']))
                        return realpath( $_ENV['TMP'] );
              if (!empty($_ENV['TMPDIR']))
                        return realpath( $_ENV['TMPDIR'] );
              if (!empty($_ENV['TEMP']))
                        return realpath( $_ENV['TEMP'] );

              $temp_file = tempnam(md5(uniqid(rand(), TRUE)), '');
             
	      if ($temp_file) {
                     $temp_dir = realpath(dirname($temp_file));
                     unlink($temp_file);
                     return $temp_dir;
              }
               return FALSE;
        } else {
                return sys_get_temp_dir();
        }
  }

try {  
       if ($tmp = getTempDir()) {
           $file = $tmp . DIRECTORY_SEPARATOR . 'pdo_uri.tst';
           $dsn = "nuodb:database=test@localhost:" . $nuotestport;
           $uri = sprintf('uri:file://%s', $file);
           
 	   if ($fp = @fopen($file, 'w')) {
               fwrite($fp, $dsn);
               fclose($fp);
               clearstatcache();
               assert(file_exists($file));
               
	       try {
                      $db = new PDO($uri, $user, $password);
        	      echo "URI connection test successful\n";
		   } catch (PDOException $e) {
		           printf("[002] URI=%s, DSN=%s, File=%s (%d bytes, '%s'), %s\n",
                           $uri, $dsn, $file, filesize($file), file_get_contents($file), $e->getMessage());
                     }
                           unlink($file);
          }
  }

$db = NULL;
} catch(PDOException $e) {  
  echo "FAILED: connect failed when it should succeed.\n";
  $db = NULL;
}

$db = NULL;  
echo "done\n";
?>
--EXPECT--
URI connection test successful
done
