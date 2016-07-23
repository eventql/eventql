3.4 JavaScript API
==================

#### Logging

The most simple script that you can run is:

    console.log(1);

Run it with:

      $ evql -f helloworld.js

#### Lazy Execution 

#### Sharing State

###### Global Variables
###### Parameters


#### Caching

EVQL will automatically cache your jobs or part of your jobs if you don't tell
it otherwise. The cache invalidation is fully automatic by default. However
it is good to understand how this works. EVQL will cache and invalidate subparts
of your job based on two pieces of information: A "checksum" that reflects the
source data on which that subpart is executed and another checksum that is
based on your source code and all broadcast variables and parameters.

This means that if you execute the same code, with the same broadcast variables
and the same parameters on the same data twice, EVQL will return a cached result
by default. This may sometimes feel strange if you have put console.log statements
into your code, since those parts of the code won't be re-executed (and hence
you won't see any log messages).


You can override this behaviour by either disabling it completely (ensuring
your code is always executed in full and nothing is ever automatically cached)
or by providing a cache key manually.


<br />
API Reference
-------------

Jump to:
  - [EVQL.log](#z1-log),
  - [EVQL.broadcast](#z1-broadcast)
  - [EVQL.mapTable](#z1-maptable)
  - [EVQL.reduce](#z1-reduce)
  - [EVQL.downloadResults](#z1-downloadresults)
  - [EVQL.saveToTable](#z1-savetotable)
  - [EVQL.processStream](#z1-processtream)
  - [EVQL.writeToOutput](#z1-writetooutput)

---
### EVQL.log

    EVQL.log(arg1, ..., argN)

Logs the arguments to STDERR. `console.log` is an alias to this method.

###### Example

    EVQL.log("blah")
    >> STDOUT: blah

    EVQL.log("blah", 123, "fubar")
    >> STDOUT: blah, 123, fubar

---
### EVQL.broadcast

Makes the specified global variable(s) available to all tasks/functions.

    EVQL.broadcast(var1 [, varN...])

Note that the arguments to this methods are the names of the variables as
strings, not the actual variables themself. Also, all broadcast variables must
be global variables (i.e. defined in the global scope).

##### Example:

    var my_constant = 123;
    EVQL.broadcast("my_constant");

    EVQL.mapTable({
      // ...
      map_fn: function (row) {
        return [[row.field1, row.field2 * my_constant]];
      }
      // ...
    });

---
### EVQL.mapTable

here be dragons

---
### EVQL.reduce

here be dragons

---
### EVQL.downloadResults

Downloads all rows in the provided sources.

    EVQL.downloadResults(sources [, serializer])

`sources` must be an array of temp tables (which are returned by methods like
`mapTable` or `reduce`). This method will download all rows from all sources and,
depending on how the script was invoked, print them to STDOUT or write them to
a result file.

To retrieve the results on stdout, invoke the script like this:

    $ zli run myscript.js

To save the results into a file, set the -o paramater

    $ zli run -o output.dat myscript.js


Optionally, this method also accepts a serializer function that determines the
formatting of the results.

##### The serializer function

The `serializer` function is called for each row before it is downloaded and
is responsible for converting the row into a string that can be written to
STDOUT/the output file.

The default serializer function will encode the key and value as a JSON
object and then return it. It looks like this:

    function serializer(key, value) {
      var json = JSON.stringify({ key: key, value: value })
      EVQL.writeToOutput(json + "\n");
    }

##### Example

    var tmp = EVQL.mapTable({
      // ...
    });

    EVQL.downloadResults([tmp]);

##### Return Value

This method does not return anything. If you want to retrieve the results of
a job, consider using `EVQL.execute`

---
### EVQL.saveToTable

Saves all rows in `sources` to `table`.

    EVQL.saveToTable(opts)

`opts` must be an object with the properties `table`, the name of the to save the results to, and `sources`,
an array of temp tables (which are returned by methods like `mapTable` or `reduce`)

    var tmp = EVQL.mapTable({
      //
    });

    EVQL.saveToTable({
      table: "my_stats",
      sources: tmp
    });

here be dragons

---
### EVQL.saveToTablePartition

here be dragons

---
### EVQL.writeToOutput

This method allows you to manually write to the output file or STDOUT of the
script (console.log will write to STDERR). This can be used to e.g. the
results of a computation as a CSV
