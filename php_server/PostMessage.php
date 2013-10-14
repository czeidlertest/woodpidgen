<?php

include 'database.php'; 


$repoPath = 'test.git';

$messenger = new MessageFeeder($repoPath);
$messenger->init();
$attachments = array();
$messenger->addMessage("header".rand(0,100), "Hallo World".rand(0,100), $attachments);

/*
$repo = new Git($repoPath);

$object = new GitBlob($repo);

$object->data = "test string";

$object->rehash();
$object->write();


$name = $object->getName();

$obj = $repo->getObject($name);
echo $obj->data;
echo "<br>";

echo "Tree test:<br>";
$tree = new GitTree($repo);
$tree->updateNode("data", 100644, $name);
$tree->updateNode("testFile", 100644, $name);

$tree->rehash();
$tree->write();

$name = $tree->getName();
$obj = $repo->getObject($name);

$treeArray = $tree->listRecursive();

foreach ($treeArray as $key => $value)
{
	echo "$key = ".sha1_hex($repo->getObject($value)->getName())."<br>";
}
*/

?>
