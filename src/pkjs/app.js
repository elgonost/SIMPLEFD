var emailReq = new XMLHttpRequest();
var activityReq = new XMLHttpRequest();
var sendData = {};
var username = localStorage.getItem("username");
var email = localStorage.getItem("email");
var threshold = localStorage.getItem("threshold");
var sensitivity = localStorage.getItem("sensitivity");
var pointThreshold = localStorage.getItem("pointThreshold");
var step = localStorage.getItem("step");
var lat = 0.0;
var long = 0.0;
var location = "";

console.log('Hello from phone');

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
  
  console.log("RECEIVED THE FOLLOWING DATA FROM CONFIG");
  if(configData.userName){
     username = configData.userName;
     localStorage.setItem("username",username);
     console.log("username set: "+username);
  }
  
  if(configData.email){
    email = configData.email;
    localStorage.setItem("email",email);
    console.log("email set: "+email);
  }
  
  threshold = configData.threshold;
  localStorage.setItem("threshold",threshold);
  console.log("threshold set: "+threshold);
  
  sensitivity = configData.sensitivity;
  localStorage.setItem("sensitivity",sensitivity);
  console.log("sensitivity set: "+sensitivity);
  
  pointThreshold = configData.pointThreshold;
  localStorage.setItem("pointThreshold",pointThreshold);
  console.log("pointThreshold set: "+pointThreshold);
  
  step = configData.step;
  localStorage.setItem("step",step);
  console.log("step set: "+step);
  
  //send options to C side
  Pebble.sendAppMessage({"thresholdKey":threshold,"sensitivityKey":sensitivity,"pointThresholdKey":pointThreshold,"stepKey":step}, messageSuccessHandler, messageFailureHandler);
});

// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendMessage() {
	Pebble.sendAppMessage({"status": 1}, messageSuccessHandler, messageFailureHandler);
  console.log('sending: '+threshold+' and '+ sensitivity);
  Pebble.sendAppMessage({"thresholdKey":threshold,"sensitivityKey":sensitivity,"pointThresholdKey":pointThreshold,"stepKey":step}, messageSuccessHandler, messageFailureHandler);
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
  if(e.payload.typeKey !== undefined){
    console.log("JAVASCRIPT HERE: RECEIVED STATE CHANGE");
    console.log('JAVACRIPT: Timekey0 = '+e.payload.timeKey0);
    console.log('JAVACRIPT: Timekey1 = '+e.payload.timeKey1);
    console.log('JAVACRIPT: Type = '+e.payload.typeKey);
    var timestamp = (e.payload.timeKey0)*100000000 + e.payload.timeKey1;
    var type = e.payload.typeKey;
    console.log('Timestamp = '+timestamp);
    console.log('type = '+type);
    sendData = {"user":username,"type":type,"timestamp1":timestamp,"timestamp2":timestamp};
    activityReq.open('POST','http://83.212.115.163:8080/simple');
    activityReq.setRequestHeader('Content-Type','application/json; charset=utf-8');
    activityReq.send(JSON.stringify(sendData));
  }
});