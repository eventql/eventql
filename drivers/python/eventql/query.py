# Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
# Authors:
#   - Laura Schlimmer <laura@eventql.io>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License ("the license") as
# published by the Free Software Foundation, either version 3 of the License,
# or any later version.
#
# In accordance with Section 7(e) of the license, the licensing of the Program
# under the license does not imply a trademark license. Therefore any rights,
# title and interest in our trademarks remain entirely with us.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the license for more details.
#
# You can be released from the requirements of the license by purchasing a
# commercial license. Buying such a license is mandatory as soon as you develop
# commercial activities involving this program without disclosing the source
# code of your own applications

import requests
import json

class Query:

    API_PATH = '/api/v1/sql'

    def __init__(self, client, query_str):
        self.client = client;
        self.query_str = query_str

    def execute(self):
        headers = {
          'Content-Type': 'application/json'
        }

        if self.client.auth_provider.has_auth_token():
          headers['Authorization'] = "Token {0}".format(
              self.client.auth_provider.auth_token)

        body = {
          'query': self.query_str,
          'format': 'json'
        }

        if self.client.auth_provider.has_database():
            body['database'] = self.client.auth_provider.database

        url = "http://{0}:{1}{2}".format(
            self.client.host,
            self.client.port,
            self.API_PATH)

        r = requests.post(url, data=json.dumps(body), headers=headers)

        if r.status_code != 200:
          raise Exception(
              "HTTP Error ({0}): {1}".format(r.status_code, r.content))

        response_json = json.loads(r.content)
        if response_json.has_key('error'):
          raise Exception("Query Error: {0}".format(response_json['error']))

        return response_json
