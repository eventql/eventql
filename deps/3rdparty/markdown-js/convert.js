#!usr/bin/node

var fs = require('fs');
var path = require('path');
var marked = require('marked');

if(process.argv.length != 4) {
  console.log("Usage: node convert.js INPUTFILE OUTPUTFILE");
}

var input_file = process.argv[2];
var output_file = process.argv[3];

fs.readFile(input_file, "utf-8", function(err, data) {
  if (err) throw err;

  // Replace ##h2 with ## h2 etc.. Javascript does not support regex lookbehind
  // and a function here would only save a line or two and be harder to understand.
  var formattedData = data
    .replace(/^[ ]*#(?=[^# ])/mg, '# ')
    .replace(/^[ ]*#{2}(?=[^# ])/mg, '## ')
    .replace(/^[ ]*#{3}(?=[^# ])/mg, '### ')
    .replace(/^[ ]*#{4}(?=[^# ])/mg, '#### ')
    .replace(/^[ ]*#{5}(?=[^# ])/mg, '##### ')
    .replace(/^[ ]*#{6}(?=[^# ])/mg, '###### ');

  var html = marked(formattedData);
  fs.writeFile(output_file, html, function(err) {
    if (err) {
      console.log(err);
      process.exit(1);
    }
  });
});
