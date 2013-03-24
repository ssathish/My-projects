<?php session_start();?>
<html>
    <head>
        <link rel="stylesheet" href="style.css" type="text/css" media="screen" />
		<!--base href="http://127.0.0.1:85/" target="_blank"-->
        <title>Academic Calender</title>
    </head>
    <body>
        <div id="container_demo">
            <div id="wrapper">
                <h1>Academic Calender</h1>
                <p class="change_link">This Application uses Google Calender account activation.<br/>Click the button below to enter the application using your Google account.</p>
                <p class="field">
                    <button name="submit" class="modern" onclick="window.location='https://accounts.google.com/o/oauth2/auth?client_id=962046158031.apps.googleusercontent.com&response_type=token&redirect_uri=http%3A%2F%2Flocalhost%3A85%2Findex.php&scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fuserinfo.email+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fuserinfo.profile'">Login using Google Account</i></button>
                </p>
                <script>
                    var query = location.href.split('#');
                    var accessStatus = query[1];
                    if(accessStatus == "error=access_denied")
                    {
                        document.writeln("<p class='alert'>!!!ALERT!!!!<br/>You have denied acces to the Application.<br/>!!!ALERT!!!</p>");
                    }
                    else if(accessStatus.match(/access_token/g) == "access_token")
                    {
			var token = query[1].split('=');
			var access_token = token[1];
			access_token =access_token.split("&");
			window.location.href="event_add.php?t="+access_token[0];
		    }
                </script>
    <?php //include('sidebar.php'); ?>
    </body>
</html>