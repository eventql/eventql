# Sending Data from the Web / with Javascript

## Gettting Started

To start ZBase tracking, you must paste the code snippet below to your page.
It will activate the ZBase tracking by creating a `ztrack` method and loading the ztrack tracking code into the page.

```
  (function(c,e,w,d,n){w=window,d=w.document,n="ztrack",w[n]=w[n]||function(){
    (w[n].q=w[n].q||[]).push(arguments)},w[n].c=c,e=d.createElement('script'),
    e.async=1,e.src='//zsrpc.net/api-'+c.id+'.js',d.head.appendChild(e)})({
      id: "<TRACK-ID>"
  });

  ztrack("page_view", {});
```


<br />
## Sending Events

In order to send an event, you must call the `ztrack` method. There are two mandatory parameters `event_type` and `event_data`:
The event_type must contain your event s type, this can be any built-in or custom defined event, and the event_data parameter
must contain the data, represented as a JSON Object, to send.

#### Syntax
```
ztrack(event_type, event_data);
```

   

#### Example
```
ztrack("my_event", {attr1: "value1", attr2: "value2"});
```


<br />
### Built-in Events


Built-in events have standard attributes that are either set automatically are must be set by you.



ZBase tracking provides the following built-in events:

##### _session
   

Attributes:
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Automatically set</th>
  </tr>
  <tr>
    <td>screen_height</td>
    <td>the height of the screen in pixels</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>screen_width</td>
    <td>the width of the screen in pixels</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>referrer_url</td>
    <td>the URL of the page that linked to this page</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>user_agent</td>
    <td>the user-agent string sent by the browser</td>
    <td>Yes</td>
  </tr>
</table>
   
   
<br />
##### page_view
Attributes
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Automatically set</th>
  </tr>
  <tr>
    <td>item_id</td>
    <td>the seen item s id</td>
    <td>No</td>
  <tr>
    <td>page_url</td>
    <td>the current pages URL</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>referrer_url</td>
    <td>the URL of the page that linked to this page</td>
    <td>Yes</td>
  </tr>
</table>


<br />
##### search_query
Attributes
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Automatically set</th>
  </tr>
  <tr>
    <td>result_item.item_id</td>
    <td></td>
    <td>No</td>
  </tr>
</table>

<br />
##### Customize
You can add any custom attribute or overwrite automatically set attributes by adding the attribute to the event s data you want to send.
   

For example add a `referrer_name` to the `_session` event:
```
ztrack("_session", {"referrer_name": "example.com"});
```

<br />
### Custom Events
You can send any custom event, that has been previously defined.   
For a full list of all events or to change and add custom events, please see the [session tracking settings](http://zbase.io/a/settings/session_tracking)

Example
```
ztrack("my_custom_event", {shop_id: 'xxx'});
```

