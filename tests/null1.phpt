--TEST--
null1 -- test text with nul byte.
--FILE--
<?php 

require("testdb.inc");
global $db;  
open_db();

try {

  $sql0 = 'DROP TABLE "simpletest310058menu_router" CASCADE IF EXISTS';
  $stmt0 = $db->prepare($sql0);
  $stmt0->execute();

  $sql1 ='CREATE TABLE "simpletest310058menu_router" ( 
	"path" VARCHAR(255) NOT NULL DEFAULT \'\',
	"load_functions" BLOB NOT NULL,
	"to_arg_functions" BLOB NOT NULL,
	"access_callback" VARCHAR(255) NOT NULL DEFAULT \'\',
	"access_arguments" BLOB NULL DEFAULT NULL,
	"page_callback" VARCHAR(255) NOT NULL DEFAULT \'\',
	"page_arguments" BLOB NULL DEFAULT NULL,
	"delivery_callback" VARCHAR(255) NOT NULL DEFAULT \'\',
	"fit" INT NOT NULL DEFAULT 0,
	"number_parts" SMALLINT NOT NULL DEFAULT 0,
	"context" INT NOT NULL DEFAULT 0,
	"tab_parent" VARCHAR(255) NOT NULL DEFAULT \'\',
	"tab_root" VARCHAR(255) NOT NULL DEFAULT \'\',
	"title" VARCHAR(255) NOT NULL DEFAULT \'\',
	"title_callback" VARCHAR(255) NOT NULL DEFAULT \'\',
	"title_arguments" VARCHAR(255) NOT NULL DEFAULT \'\',
	"theme_callback" VARCHAR(255) NOT NULL DEFAULT \'\',
	"theme_arguments" VARCHAR(255) NOT NULL DEFAULT \'\',
	"type" INT NOT NULL DEFAULT 0,
	"description" TEXT NOT NULL,
	"position" VARCHAR(255) NOT NULL DEFAULT \'\',
	"weight" INT NOT NULL DEFAULT 0,
	"include_file" TEXT DEFAULT NULL,
	PRIMARY KEY (path) )';
  $stmt1 = $db->prepare($sql1);
  $stmt1->execute();

  $sql2 = 'CREATE INDEX "simpletest310058menu_router_fit_idx" ON simpletest310058menu_router ("fit")';
  $stmt2 = $db->prepare($sql2);
  $stmt2->execute();

  $sql3 = 'CREATE INDEX "simpletest310058menu_router_tab_parent_idx" ON simpletest310058menu_router ("tab_parent"(64), "weight", "title")';
  $stmt3 = $db->prepare($sql3);
  $stmt3->execute();

  $sql4 = 'CREATE INDEX "simpletest310058menu_router_tab_root_weight_title_idx" ON simpletest310058menu_router ("tab_root"(64), "weight", "title")';
  $stmt4 = $db->prepare($sql4);
  $stmt4->execute();

  $sql5 = 'INSERT INTO simpletest310058menu_router ("path", "load_functions", "to_arg_functions", "access_callback", "access_arguments", "page_callback", "page_arguments", "delivery_callback", "fit", "number_parts", "context", "tab_parent", "tab_root", "title", "title_callback", "title_arguments", "theme_callback", "theme_arguments", "type", "description", "position", "weight", "include_file") VALUES (:db_insert_placeholder_0, :db_insert_placeholder_1, :db_insert_placeholder_2, :db_insert_placeholder_3, :db_insert_placeholder_4, :db_insert_placeholder_5, :db_insert_placeholder_6, :db_insert_placeholder_7, :db_insert_placeholder_8, :db_insert_placeholder_9, :db_insert_placeholder_10, :db_insert_placeholder_11, :db_insert_placeholder_12, :db_insert_placeholder_13, :db_insert_placeholder_14, :db_insert_placeholder_15, :db_insert_placeholder_16, :db_insert_placeholder_17, :db_insert_placeholder_18, :db_insert_placeholder_19, :db_insert_placeholder_20, :db_insert_placeholder_21, :db_insert_placeholder_22)';
  $stmt5 = $db->prepare($sql5);

$db_insert_placeholder_0 = 'node/add/page';
$db_insert_placeholder_1 = '';
$db_insert_placeholder_2 = '';
$db_insert_placeholder_3 = 'node_access';
$db_insert_placeholder_4 = 'a:2:{i:0;s:6:"create";i:1;s:4:"page";}';
$db_insert_placeholder_5 = 'node_add';
$db_insert_placeholder_6 = 'a:1:{i:0;s:4:"page";}';
$db_insert_placeholder_7 = '';
$db_insert_placeholder_8 = '7';
$db_insert_placeholder_9 = '3';
$db_insert_placeholder_10 = '';
$db_insert_placeholder_11 = '';
$db_insert_placeholder_12 = 'node/add/page';
$db_insert_placeholder_13 = 'Basic page';
$db_insert_placeholder_14 = 'check_plain';
$db_insert_placeholder_15 = '';
$db_insert_placeholder_16 = '';
$db_insert_placeholder_17 = 'a:0:{}';
$db_insert_placeholder_18 = '6';
$db_insert_placeholder_19 = '';
$db_insert_placeholder_20 = '';
$db_insert_placeholder_21 = '0';
$db_insert_placeholder_22 = 'modules/node/node.pages.inc';

  $stmt5->bindParam(':db_insert_placeholder_0', $db_insert_placeholder_0, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_1', $db_insert_placeholder_1, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_2', $db_insert_placeholder_2, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_3', $db_insert_placeholder_3, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_4', $db_insert_placeholder_4, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_5', $db_insert_placeholder_5, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_6', $db_insert_placeholder_6, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_7', $db_insert_placeholder_7, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_8', $db_insert_placeholder_8, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_9', $db_insert_placeholder_9, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_10', $db_insert_placeholder_10, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_11', $db_insert_placeholder_11, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_12', $db_insert_placeholder_12, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_13', $db_insert_placeholder_13, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_14', $db_insert_placeholder_14, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_15', $db_insert_placeholder_15, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_16', $db_insert_placeholder_16, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_17', $db_insert_placeholder_17, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_18', $db_insert_placeholder_18, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_19', $db_insert_placeholder_19, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_20', $db_insert_placeholder_20, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_21', $db_insert_placeholder_21, PDO::PARAM_STR);
  $stmt5->bindParam(':db_insert_placeholder_22', $db_insert_placeholder_22, PDO::PARAM_STR);
  $stmt5->execute();
  $id = $db->lastInsertId();

  $id = $db->lastInsertId();
  print_r($id);
  

  $db = NULL;
} catch(PDOException $e) {
  echo $e->getMessage();
}
$db = NULL;
echo "done\n";

?>
--EXPECT--
done