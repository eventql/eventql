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
from query import Query

class Client:

    def __init__(self, host, port, auth_provider):
        self.host = host
        self.port = port
        self.auth_provider = auth_provider

    def query(self, query_str):
        return Query(self, query_str)

    def insert(self, data):
        headers = {
          'Content-Type': 'application/json'
        }

        if self.auth_provider.has_auth_token():
          headers['Authorization'] = "Token {0}".format(
              self.auth_provider.auth_token)

        url = "http://{0}:{1}/api/v1/tables/insert".format(
            self.host,
            self.port)
        r = requests.post(url, data=json.dumps(data), headers=headers)

        if r.status_code != 201:
          raise Exception(
              "HTTP Error ({0}): {1}".format(r.status_code, r.content))


