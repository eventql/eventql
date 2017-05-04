
from eventql.client import Client
from eventql.auth import AuthProvider
import json

auth_provider = AuthProvider()
auth_provider.set_database("test")

client = Client("localhost", 9175, auth_provider)

query = client.query("select 1;")
result = query.execute()
print(json.dumps(result))

body = [{"database": "test", "table": "test", "data": {"id": "123"}}]
client.insert(body)
