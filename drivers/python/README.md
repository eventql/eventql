# eventql-python

## Example: Executing a query

    auth_provider = AuthProvider()
    auth_provider.set_database("test")

    client = Client("localhost", 9175, auth_provider)


    query = client.query("select 1;")
    result = query.execute()


## Example: Inserting data
We use our client from the previous example to insert data into EventQL

    data = [{"database": "test", "table": "test", "data": {"id": "123"}}]
    client.insert(body)


