4.1 Functions and Operators
===========================

EventQL provides a large number of functions and operators for the built-in data types.
Users can also define their own functions and operators.

---



###### Control Flow Functions

<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/if-expression">if</a></td>
    <td>an if statement with lazy evalutation</td>
    <td><code>if(1 == 2, "foo", "bar")</code></td>
  </tr>
</table>

###### String Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/regexp-operator">REGEXP</a></td>
    <td>REGEXP operator</td>
    <td><code>'foobar' REGEXP '^foo'</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/endswith">endswith</a></td>
    <td>check string end</td>
    <td><code>endswith("eventql", "ql")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/startswith">startswith</a></td>
    <td>check string start</td>
    <td><code>startswith("eventql", "event")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/uppercase">uppercase</a></td>
    <td>convert to uppercase</td>
    <td><code>uppercase("hello world")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/ucase">ucase</a></td>
    <td>alias for uppercase</td>
    <td><code>ucase("hello world")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/lowercase">lowercase</a></td>
    <td>convert to lowercase</td>
    <td><code>lowercase("hello world")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/lcase">lcase</a></td>
    <td>alias for lowercase</td>
    <td><code>lcase("hello world")</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/substring">lcase</a></td>
    <td>Extract substring as specified.</td>
    <td><code>substring("foobar", 2, 3)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/substr">lcase</a></td>
    <td>alias for substring</td>
    <td><code>substr("foobar", 3)</code></td>
  </tr>
</table>

###### Numeric Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/add-operator">+</a></td>
    <td>Sum of two values</td>
    <td><code>2 + 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/mul-operator">*</a></td>
    <td>Multiply two numbers</td>
    <td><code>4 * 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/mul-operator">/</a></td>
    <td>Division operator</td>
    <td><code>4 / 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/pow">pow</a></td>
    <td>Power operator</td>
    <td><code>pow(2, 32)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/round">round</a></td>
    <td>Round a number</td>
    <td><code>round(0.234, 2)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/truncate">truncate</a></td>
    <td>Truncate a number</td>
    <td><code>truncate(0.234, 2)</code></td>
  </tr>
</table>

###### Boolean Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/neg-operator">!</a></td>
    <td>Logical Negation operator</td>
    <td><code>!true</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/eq-operator">==</a></td>
    <td>'Equal' operator</td>
    <td><code>2 == 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/neq-operator">!=</a></td>
    <td>'Not equal' operator</td>
    <td><code>2 != 3</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/lt-operator">&lt;</a></td>
    <td>'Less than' operator</td>
    <td><code>2 &lt; 4</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/lte-operator">&lt;=</a></td>
    <td>'Less or equal than' operator</td>
    <td><code>2 &lt;= 4</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/gt-operator">&gt;</a></td>
    <td>'Greather than' operator</td>
    <td><code>4 &gt; 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/gte-operator">&gt;=</a></td>
    <td>'Greather or equal than' operator</td>
    <td><code>4 &gt;= 2</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/and">AND</a></td>
    <td>Logical and</td>
    <td><code>1 &lt; 2 AND 2 &lt; 3</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/or">OR</a></td>
    <td>Logical or</td>
    <td><code>1 &lt; 2 OR 2 &lt; 3</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/isnull">isnull</a></td>
    <td>Check if value is null</td>
    <td><code>isnull(null)</code></td>
  </tr>
</table>

###### DateTime Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/from_timestamp">from_timestamp</a></td>
    <td>Convert a timestamp to a DateTime value</td>
    <td><code>from_timestamp(1462125626)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/date_trunc">date_trunc</a></td>
    <td>Truncate to specified precision</td>
    <td><code>date_trunc("d", 1462125626)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/date_add">date_add</a></td>
    <td>Add interval</td>
    <td><code>date_add(1462125626, '1', 'DAY')</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/time_at">time_at</a></td>
    <td>Get DateTime value for interval from now</td>
    <td><code>time_at('-12hours')</code></td>
  </tr>
</table>

###### Aggregate Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/sum">sum</a></td>
    <td>Sum of all values in the result set</td>
    <td><code>sum(price)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/count">count</a></td>
    <td>Number of values in the result set</td>
    <td><code>count(1)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/min">min</a></td>
    <td>Minimum of values in the result set</td>
    <td><code>min(price)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/max">max</a></td>
    <td>Maximum of values in the result set</td>
    <td><code>max(price)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/mean">mean</a></td>
    <td>mean of values in the result set</td>
    <td><code>mean(price)</code></td>
  </tr>
</table>

###### Conversion Functions
<table class="small functions_and_operators">
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/to_str">to_str</a></td>
    <td>Convert to string</td>
    <td><code>to_str(1)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/to_int">to_int</a></td>
    <td>Convert to integer</td>
    <td><code>to_int(142.23)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/to_float">to_float</a></td>
    <td>Convert to float</td>
    <td><code>to_float(12)</code></td>
  </tr>
  <tr>
    <td><a class="link" href="/documentation/sql/functions-and-operators/to_bool">to_bool</a></td>
    <td>Convert to string</td>
    <td><code>to_bool(1)</code></td>
  </tr>
</table>

---
##### Operator Precedence

Operator precedences are shown in the following list, from highest precedence to
the lowest. Operators that are shown together on a line have the same precedence.

    !
    - (unary minus), ~ (unary bit inversion)
    ^
    *, /, DIV, %, MOD
    -, +
    <<, >>
    &
    |
    = (comparison), <=>, >=, >, <=, <, <>, !=, IS, LIKE, REGEXP, IN
    BETWEEN, CASE, WHEN, THEN, ELSE
    NOT
    &&, AND
    XOR
    ||, OR

