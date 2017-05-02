const AuthProvider = require('./lib/AuthProvider');
const Client = require('./lib/Client');


const auth = new AuthProvider;
auth.setDatabase("test");

let client = new Client('localhost', 9175, auth);

client.insert([{
  "database": "test",
  "table": "test",
  "data": {
    name: 'fred',
    value: 'ab',
    time: Date.now()
  }
}], (error) => {
  if (error) {
    console.log(error);
    console.log("An error occured while creating the table: ${error.message}");
  }
})

let query = client.query("select * from test");
query.execute((results) => {
  console.log(results);
}, (error) => {
  console.log(error);
});

query.executeSSE({
  onProgress: function(progress) {
    //do stuff here
    console.log(progress);
  },
  onResult: function(results) {
    //do stuff here
    console.log(results);
  },
  onError: function(type, message) {
    //do stuff here
    console.log(type, message);
  }
});
