document.getElementById('Run').addEventListener('click', cameras);
document.getElementById('save-button').addEventListener('click', custom_speeds);
document.getElementById('Select_hemisphere').addEventListener('change', hemispherer);
document.getElementById('slewing-speed').addEventListener('keypress', slew);
document.getElementById('move-left').addEventListener('mousedown', left);
document.getElementById('move-right').addEventListener('mousedown', right);
document.getElementById('move-left'||'move-right').addEventListener('mouseup', reelease);
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
moveright = true;
moveleft = true ;
releas = true;
hemisphere = document.getElementById('Select_hemisphere');
hemisphere.addEventListener("change", () =>{
  if  (hemisphere.value = "north") {
    NS = true;
  }
  else {
    NS = false;
  }
})
function reelease(){
  const isreleas = {
    meybe : releas,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(isreleas))
    console.log('slew speed sent');
  }
}
function slew(){
  const slewing = {
    slewe : document.getElementById('slewing-speed').value,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(slewing))
    console.log('slew speed sent');
  }
}
function right(){
  const right_move ={
    righton : moveright,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(right_move))
    console.log('right move sent');
  }
}
function left(){
  const left_move ={
    lefton : moveleft,
  };
  if (isopen = true) {
    webSocket.send(JSON.stringify(left_move))
    console.log('left move sent');
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
