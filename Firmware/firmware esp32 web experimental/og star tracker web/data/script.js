document.getElementById('Run').addEventListener('click', cameras);
document.getElementById('save-button').addEventListener('click', custom_speeds);
document.getElementById('Select_hemisphere').addEventListener('change', hemispherer);
document.getElementById('slewing-speed').addEventListener('submit', hemispherer);
const gateway = `ws://${window.location.hostname}/ws`;
let webSocket; // Declare webSocket as a global variable
isopen = false ;
function connectToWebSocket() {
  webSocket = new WebSocket(gateway);

  webSocket.addEventListener('open', function (event) {
    console.log('WebSocket connection opened');
    isopen = true ;
  });

  webSocket.addEventListener('message', function (event) {
    console.log('Received message:', event.data);
  });

  webSocket.addEventListener('close', function (event) {
    console.log('WebSocket connection closed');
  });

  webSocket.addEventListener('error', function (event) {
    console.error('WebSocket error:', event);
  });

  return webSocket;
}
NS = false;
moveright = false;
hemisphere = document.getElementById('Select_hemisphere');
hemisphere.addEventListener("change", () =>{
  if  (hemisphere.value = "north") {
    NS = true;
  }
  else {
    NS = false;
  }
})
function slew(){
  const slewing = {
    slewe : document.getElementById('slewing-speed').value,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(slewing))
  }
}
function right(){

  const right_move ={
    righton : moveright,
  }

}
function cameras(){
  const camera = {
    exposures: document.getElementById('num-exposures').value,
    length: document.getElementById('exposure-length').value,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(camera))
  }
}
function custom_speeds(){
  const custom_speed = {
    speed : document.getElementById("option-input").value,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(custom_speed));
  }
}
function hemispherer(){
  const hem = {
    hemisphereNS : NS,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(hem));
  }
}
window.onload = function(event) {
  connectToWebSocket();
}
