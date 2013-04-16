<?php 
try {  
  $db = new PDO("nuodb:database=test@localhost;schema=Hockey", "dba", "goalie") or die;
  $number = 12;
  $sql = "select * from HOCKEY where NUMBER < :number";
  $stmt = $db->prepare($sql);
  $stmt->bindParam(':number', $number, PDO::PARAM_INT);
  $stmt->execute();
  $result = $stmt->fetchAll();
  foreach ($result as $row) {
     echo "NUMBER=" . $row["NUMBER"] . " NAME=" . $row["NAME"] . "\n";
  }
  $db = NULL;
} catch(PDOException $e) {  
  echo $e->getMessage();  
} $db = NULL;  
echo "done\n";
?>
