var emailReq = new XMLHttpRequest();
var username = localStorage.getItem("username");
var email = localStorage.getItem("email");
var lat = 0.0;
var long = 0.0;
var location = "";

function geolocationSuccess(position){
  lat = position.coords.latitude;
  long = position.coords.longitude;
  console.log("Latitude: "+ lat + " longitude: " + long);
  location = "Latitude: "+ lat + " longitude: " + long;
}

function geolocationError(error) {
  console.log("code: "    + error.code    + "\n" + "message:" + error.message + "\n");
}


Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://83.212.115.163/config.html';
  console.log('Showing configuration page: '+ url);
  Pebble.openURL(url);
});


Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configData = JSON.parse(decodeURIComponent(e.response));
  username = configData.userName;
  localStorage.setItem("username",username);
  console.log("username set: "+username);
  
  email = configData.email;
  localStorage.setItem("email",email);
  console.log("email set: "+email);
});

// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendMessage() {
	Pebble.sendAppMessage({"status": 1}, messageSuccessHandler, messageFailureHandler);
}

// Called when the message send attempt succeeds
function messageSuccessHandler() {
  console.log("Message send succeeded.");  
}

// Called when the message send attempt fails
function messageFailureHandler() {
  console.log("Message send failed.");
  sendMessage();
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  console.log("JS is ready!");
  sendMessage();
});
												
// Called when incoming message from the Pebble is received
// We are currently only checking the "message" appKey defined in appinfo.json/Settings
Pebble.addEventListener("appmessage", function(e) {
  
  navigator.geolocation.getCurrentPosition(geolocationSuccess,[geolocationError]);
  console.log(location);
  
  if(e.payload.fallKey !== undefined){
     //send email
     console.log("JAVASCRIPT HERE: sending email");
     //navigator.geolocation.getCurrentPosition(geolocationSuccess,[geolocationError]);
     //emailData = username+" needs help ASAP. His location is: "+location;
     //console.log(emailData);
  
     emailReq.open('POST','http://83.212.115.163/EmailSender.php?username='+username+'&latitude='+lat+'&longitude='+long+'&email='+email,true);
     emailReq.setRequestHeader('Content-Type', 'application/json; charset=utf-8');   
     emailReq.send();
      
    }
});