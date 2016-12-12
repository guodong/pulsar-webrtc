var net = require('net');
var ws = require('ws');
//var wsc = new ws('ws://switch.cloudwarehub.com/?type=server&token=123_conn');
var wsc = new ws(process.env.SWITCH);
wsc.on('open', function(){
  wsc.send('ready');
});
var client = net.createConnection("/tmp/mysocket", function() {
});
wsc.on('message', function(data, flags){
  console.log(data);
  client.write(data);
});

client.on("connect", function() {
  console.log('conned');
});

client.on("data", function(data) {
  console.log(data.toString());
  wsc.send(data.toString());
});

