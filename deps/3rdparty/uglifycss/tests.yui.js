#! /usr/bin/env node

(function () {

    "use strict";

    // path to yui tests
    var PATH = "../yuicompressor/tests";

    // dependancies
    var fs = require("fs");
    var uglifycss = require("./uglifycss-lib");

    // trim results (some ref minified files got trailing new lines)
    function trim(str) {
        return str.toString().replace(/(^\s*|\s*$)/g, "");
    }

    // get sorted files list and init counters
    var files = fs.readdirSync(PATH).sort();
    var failed = 0;
    var total = 0;

    // remove previous failures
    files.forEach(function (file) {
        if (/\.FAILED$/.test(file)) {
            fs.unlink(PATH + "/" + file);
        }
    });

    // check files
    files.forEach(function (file) {
        file = PATH + "/" + file;
        if (/\.css$/.test(file)) {
            var ugly = uglifycss.processFiles([ file ]);
            if (trim(ugly) !== trim(fs.readFileSync(file + ".min"))) {
                console.log(file + ": FAILED");
                fs.writeFile(file + ".FAILED", ugly);
                failed += 1;
            }
            total += 1;
        }
    });

    // report total
    if (failed) {
        console.log(total + " tests, " + failed + " failed");
    } else {
        console.log(total + " tests, no failure!");
    }

}());
