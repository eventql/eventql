chartsql
========

embeddable C++ SQL runtime, supports ChartSQL extensions

### Install From Source: Ubuntu

Requires cmake

```
git clone git@github.com:paulasmuth/chartsql.git
cd chartsql
make
```


Examples
--------

Most simple example: evaluate a constant expression without inputs:

```C++
#include <csql/csql.h>
#include <string>

int main() {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  std::string expression = "1 + 2";
  auto res = runtime->evaluateConstExpression(txn.get(), expression);
  std::string res_str = v.toString();

  printf("result: %s\n", re_str.c_str()); // prints "result: 3"
  return 0;
}
```

LICENSE
-------

```
Copyright (c) 2015 Paul Asmuth

libstx is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

libstx is distributed in the hope that it will be useful,but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
libstx. If not, see <http://www.gnu.org/licenses/>.
