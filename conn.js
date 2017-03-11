var net = require('net');
var WebSocket = require('ws');
var dgram = require('dgram');
var client = dgram.createSocket('udp4');
var port = process.argv[2] || 8080;

var wss = new WebSocket.Server({
  port: port
});

wss.on('connection', function(ws) {
  ws.on('message', function(msg) {
    console.log(msg);
    var data = new Buffer(msg);
    client.send(data, 0, data.length, port, 'localhost');
  });

  client.on("message", function(data) {
    console.log(data.toString());
    ws.send(data.toString());
  });
})



