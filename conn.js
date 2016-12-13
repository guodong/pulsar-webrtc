var net = require('net');
var ws = require('ws');
var dgram = require('dgram');
var client = dgram.createSocket('udp4');
var wsc = new ws('ws://switch.cloudwarehub.com/?type=server&token=123_conn');
//var wsc = new ws(process.env.SWITCH);
wsc.on('open', function(){
  wsc.send('ready');
});

wsc.on('message', function(data, flags){
  console.log(data);
  client.send(data, 8888, 'localhost');
});

client.on("message", function(data) {
  console.log(data.toString());
  wsc.send(data.toString());
});

