var Socket;
document.getElementById('Run').addEventListener('click', runit);
function init() {
    Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    Socket.onmessage = function(event) {
      runit(event);
    };
}
function runit() {
    var camera = {
        exposures: document.getElementById('num-exposures').value,
        length: document.getElementById('exposure-length').value,
	};
	Socket.send(JSON.stringify(camera));
}
window.onload = function(event) {
    init();
}