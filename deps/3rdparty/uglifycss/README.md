UglifyCSS is a port of [YUI Compressor](https://github.com/yui/yuicompressor) to [NodeJS](http://nodejs.org) for its CSS part. Its name is a reference to the awesome [UglifyJS](https://github.com/mishoo/UglifyJS) but UglifyCSS is not a CSS parser. Like YUI CSS Compressor, it applies many regexp replacements. Note that a [port to JavaScript](https://github.com/yui/ycssmin) is also available in the YUI Compressor repository.

UglifyCSS passes successfully the test suite of YUI compressor CSS.

### Installation

For a command line usage:
```sh
$ npm install uglifycss -g
```

For API usage:
```sh
$ npm install uglifycss
```

From Github:
```sh
$ git clone git://github.com/fmarcia/UglifyCSS.git
```

### Command line

```sh
$ uglifycss [options] [filename] [...] > output
```

Options:

* `--max-line-len n` adds a newline (approx.) every `n` characters; `0` means no newline and is the default value
* `--expand-vars` expands variables; by default, `@variables` blocks are preserved and `var(x)`s are not expanded
* `--ugly-comments` removes newlines within preserved comments; by default, newlines are preserved
* `--cute-comments` preserves newlines within and around preserved comments

If no file name is specified, input is read from stdin.

### API

2 functions are provided:

* `processString( content, options )` to process a given string
* `processFiles( [ filename1, ... ], options )` to process the concatenation of given files

Options are identical to the command line:
* `<int> maxLineLen` for `--max-line-len n`
* `<bool> expandVars` for `--expand-vars`
* `<bool> uglyComments` for `--ugly-comments`
* `<bool> cuteComments` for `--cute-comments`

Both functions return uglified css.

#### Example

```js
var uglifycss = require('uglifycss');

var uglified = uglifycss.processFiles(
    [ 'file1', 'file2' ],
    { maxLineLen: 500, expandVars: true }
);

console.log(uglified);
```

See also [test.js](https://github.com/fmarcia/UglifyCSS/blob/master/test.js).

### License

UglifyCSS is MIT licensed.

