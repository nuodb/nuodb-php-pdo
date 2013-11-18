--TEST--
DB-4741 -- fathom select
--FILE--
<?php 
date_default_timezone_set('America/New_York');

require("testdb.inc");
global $db;  
open_db();

try {  
  $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

  $sql0 = 'DROP TABLE blobtest CASCADE IF EXISTS';
  $db->query($sql0);

  $sql1 = "create table blobtest (id int, cardNumber BLOB NOT NULL DEFAULT '')";
  $db->query($sql1);


  $key = 'asdfasdfasdfsadf'; //Change the key here
  $td = mcrypt_module_open('tripledes', '', 'cfb', '');
  srand((double) microtime() * 1000000);
  $iv = mcrypt_create_iv(mcrypt_enc_get_iv_size($td), MCRYPT_RAND);
  $okey = substr(md5($key.rand(0, 9)), 0, mcrypt_enc_get_key_size($td));
  mcrypt_generic_init($td, $okey, $iv);
  $creditno='1234567812345674';
  $encrypted = mcrypt_generic($td, $creditno.chr(194));
  $code = $encrypted.$iv;
  $code = str_replace("'", "\'", $code);

  $query = "insert into blobtest (id, cardNumber) values (:id, :cardNumber)";
  $qh=$db->prepare($query);

  $id = 1;
  $qh->bindParam(':id', $id, PDO::PARAM_INT);
  $qh->bindParam(':cardNumber', $code, PDO::PARAM_LOB); 
  $qh->execute();

  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
echo "\ndone\n";
?>
--EXPECT--
done
