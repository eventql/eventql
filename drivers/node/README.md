# EventQL Node driver

## AuthProvider

The AuthProvider class has methods to set the information needed to authenticate with the EventQL server.

    AuthProvider()

    setDatabase(database)
    setAuthToken(auth_token)
    setPassword(password)
    setUser(username)

## Client

    Client(host, port, auth_provider)

Example:

    let auth_provider = new AuthProvider;
    auth_provider.setDatabase("my_db");

    let client = new Client("localhost", 9175, auth_provider);


The insert method allows you to directly insert data into EventQL

    insert(body, callback)

Example:

    client.insert([{
      "database": "test",
      "table": "test",
      "data": {
        value: 123112,
        time: Date.now()
      }
    }], (error) => {
      if (error) {
        console.log(error);
        console.log("An error occured while creating the table: ${error.message}");
      }
    })


The query method returns a Query instance 

    query(query_str)


## Query

    Query(query_str)

The query is executed and either the success_callback with the JSON result or the error_callback is called
    execute(success_callback, error_callback)

Example:

    let query = client.query("select * from test");
    query.execute((results) => {
      // 
    }, (error) => {

    });

The query is executed via Server-sent events so that the progress of the query execution is continously reported
    executeSSE(success_callback, error_callback)

    query.executeSSE({
      onProgress: function(progress) {

      },
      onResult: function(results) { 

      },
      onError: function(type, message) {

      }
    });
