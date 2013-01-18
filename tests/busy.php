<?php 
$fanCount++;
for ($i=0; $i<1000; $i++) {
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $sql = "select * from hockey";
  foreach ($db->query($sql) as $row) {
     if ($row['POSITION'] == 'Fan')
       $fanCount++;
  }
  $db = NULL;
  print $i;
} catch(PDOException $e) {  
  echo $e->getMessage();  
}
$db = NULL;  
}
echo "done\n";
?>
