<!DOCTYPE html>
<html lang="en">
    <head>
        <title>Add Events</title>
        <link rel="stylesheet" type="text/css" href="eventFormStyle.css" />
        <link rel="stylesheet" href="style.css" type="text/css" media="screen" />
    </head>
    <body>
		<div class="wrapper">			
				<h3>Create a Project Event</h3>

				<form class="form" id="form" action="project.php" method="get">

					<div class="outer">
						<div class="inner">
							<label for="input-title">Project/Event Name</label>
							<input type="text" name="title" id="input-title" required>
						</div>
					</div>

					<div class="outer">
						<div class="inner">
							<label for="input-desc">Description of the Event</label>
							<input type="text" name="fullname" id="input-desc">
						</div>
					</div>

					<div class="outer">
						<div class="inner">
						  <label for="input-sdate">Start Date</label>
						  <input type="date" name="startDate" id="input-sdate" placeholder="MM/DD/YYYY" required>
						</div>
					</div>

					<div class="outer">
						<div class="inner">
							<label for="input-edate">End Date</label>
							<input type="date" name="enddate" id="input-edate" placeholder="MM/DD/YYYY" required>
						</div>
					</div>

					<div class="outer">
						<div class="inner">
						  <label for="input-milestone">Number of Milestones</label>
						  <input type="text" name="milestoneCount" id="input-milestone" required>
						</div>
					</div>					
					<input type="submit" value="Create Project Event" /> 

				</form>
			</div>
     </body>
</html>