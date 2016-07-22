4.1 Config File
=====================

Configuration files are a convenient way to specifiy options that you use regurlay when running EventQL programs, so that you don't have to enter them on the command line each time you run the program.
The settings are equivalent to the program's command line options (long opt). <br>
To learn more about the particular program options, please refer to the [evql](../evql), [evqld](../evqld) and [evqlctl](../evqlctl) documentation pages.

Any config file option can be overwritten on the command line by providing the equivalent key value pair.

####File Format
EventQL uses the INI file format.

    [evql]
    host = prod.example.com ; execute queries on this server


####File path
By default EventQL tries to read the configuration from `/etc/{process}.conf` and `~/.evqlrc`.
You can specify a custom file path to read your configuration from by using the command line option `-c file_path`.

