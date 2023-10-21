<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title></title>
</head>
<body>

<?php 
$ip="123";
?>
<h1>ArtnetToDMX Configuration</h1>

<form action="/submit.php" method="post">
	<label for="ip_address">IP Address:</label><br>
	<input type="text" name="ip_address" value="<?php echo $ip?>"><br>

	<label for="subnet_mask">Subnet:</label><br>
	<input type="text" name="subnet_mask"><br>

	<label for="mac_address">MAC Address:</label><br>
	<input type="text" name="mac_address"><br>

	<input type="submit" formmethod="post" value="Submit">
</form>



</body>
</html>
