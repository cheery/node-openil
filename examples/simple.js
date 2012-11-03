var il = require('openil');
var image = il.loadSync(__dirname + '/cat.png');
il.saveSync(__dirname + '/cat.jpg', image);
