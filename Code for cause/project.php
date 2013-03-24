<?php 
echo "Connect to database";
$con= mysql_connect("localhost","root","admin1");
if(!$con)
{
	mysql_close($con);
	die('Could not connect');
}
else
{
	echo "Connect successful";
}

$result= mysql_query('select * from acadplanner.students');

while ($row=mysql_fetch_array($result))
{
	echo $row[SID]. " " . $row["Sname"]. " " . $row["Email"];
}

$row[SID]=107;
$row["Sname"]='Sathish';
$row["Email"]='sathishs@usc.edu';

$query="insert into acadplanner.students values($row[SID],'$row[Sname]','$row[Email]')";

if(!mysql_query($query,$con))
{
	echo "insert failed";
}
else
{
	echo "insert success";
} 
mysql_close($con);
?>