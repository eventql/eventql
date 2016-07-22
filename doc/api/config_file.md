4.1 Config File
=====================

Configuration files are a convenient way to specifiy options that you use regurlay when running EventQL programs, so that you don't have to enter them on the command line each time you run the program.
The settings are equivalent to the program's command line options (long opt). <br>
To learn more about the particular program options, please refer to the [evql](../evql), [evqld](../evqld) and [evqlctl](../evqlctl) documentation pages.

Any config file option can be overwritten on the command line by providing the equivalent key value pair.






- /etc/{process}.conf oder ~/.evqlrc oder file path bei -c angeben, dann wird config file aus diesem file gelesen


format ini file
segmented into sections, variables as key = value, values can be enclosed with quotes 
comments can be added at the end of a variable line, separated from the key value pair by a semicolon 

[evql]
host = prod.example.net

[evqld]

[evqlctl]


