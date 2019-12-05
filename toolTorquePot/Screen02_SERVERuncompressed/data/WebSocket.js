var set = 0;
var connection;
function wsConnect(){
	connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
	connection.onopen = function () {
		connection.send('Connect ' + new Date());
	};
	connection.onerror = function (error) {
		console.log('WebSocket Error ', error);
		connection.close();
	};
	connection.onmessage = function (e) { 
		console.log(e);
		var d = JSON.parse(e.data);
		console.log(d);
		for (key in d){
			switch(key){
				case 'torque':
					document.getElementById('torque').value = d[key];
					updateTorqueBar(d.torque);
					break;
				case 'model':
					document.getElementById('model').value = d[key];
					break;
				case 'chuck':
					document.getElementById('chuck').value = d[key];
					break;
				case 'set':
					document.getElementById('set').value = d[key];
					set = d[key];
					break;
				default:
					console.log(key);
			}
		}
	};
	connection.onclose = function(){
		console.log('WebSocket connection closed');
		setTimeout(function(){
			wsConnect();
		}, 1000);
	};
}

wsConnect();
/*
function cal(){
	connection.send("R");
}
*/

function updateTorqueBar(t){
	var percent = 100 * t/set;
	if (percent > 100){
		percent = 100;
		colour = "#FF0000";//RED
	} else if (percent < 0){
		percent = 0;
	} else if (percent < 75) {
		colour = "#FF0000";//RED
	} else if (percent < 90) {
		colour = "#FFA500";//ORANGE
	} else {
		colour = "#4CAF50";//GREEN
	}
	document.getElementById('torque_bar').style.width = percent + '%';
	document.getElementById('torque_bar').style.backgroundColor = colour;
	console.log(t/set);
}