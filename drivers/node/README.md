# JavaScript Interface for EventQL HTTP API

[![JavaScript Style
Guide](https://cdn.rawgit.com/feross/standard/master/badge.svg)](https://github.com/feross/standard)

It has a simple interface to create tables, insert records and run queries againt the EventQL server.

<br>
#Auth
To identify your session, EventQL needs information about the current namespace.

###constructor

    var auth = new Auth('my_db');

The constructor takes as argument the name of the database you want to run the request against.

###Setter and getter

####Auth token 

    setAuthToken(token)
    getAuthToken()

####User and password

    setUser('my_user')
    getUser();

    setPassword('my_password');
    getPassword();

####Database

    getDatabase()

<br>
#EventQL

###constructor


    const auth = new Auth('my_db');
    const eventql = new EventQL('localhost', 9175, auth);

`host` and `port` indicate the EventQL server location you want to perform the request against.
`auth` must be an isntance of class `Auth`.


###createTable

    createTable(table_name, columns, error_callback);


    eventql.createTable('sensors1', [
      {
        id: 1,
        name: 'time',
        type: 'DATETIME',
        optional: false,
        repeated: false
      },
      {
        id: 2,
        name: 'name',
        type: 'STRING',
        optional: false,
        repeated: false
      },
      {
        id: 3,
        name: 'value',
        type: 'STRING',
        optional: false,
        repeated: false
      }
    ], (error) => {
      if (error) {
        console.log(`An error occured while creating the table: ${error.message}`);
      }
    })


###insert

    insert(table_name, record, error_callback);


     eventql.insert('sensors', {
        name: 'name',
        value: 'value',
        time: Date.now()
      }, (error) => {
        if (error) {
          console.log(`An error occured while creating the table: ${error.message}`);
        }
      })

###execute

    execute(query_str, callback_options);


    eventql.execute('select 1;', {
      onProgress: function(progress) {
        //do stuff here
      },
      onResult: function(results) {
        //do stuff here
      },
      onError: function(type, message) {
        //do stuff here
      }
    });

