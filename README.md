# DevIL bindings for node.js

There apparently isn't an up-to-date image library that'd handle decoding. Therefore I decided to write bindings for openil. If you have something better, use that instead.

## Installation (debian, ubuntu)

    sudo apt-get install libdevil-dev
    npm install openil

## Usage

    var il = require('openil');
    var image = il.loadSync('cat.png');
    il.saveSync('cat.jpg', image);

Image contains attributes width, height, format, type, data. It's very limited subset of DevIL and doesn't follow the unnecessarily silly API of DevIL.
