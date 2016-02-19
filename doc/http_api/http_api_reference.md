HTTP API Reference
==================

## Tables

---
### POST /api/v1/tables/insert

insert rows into an existing table. the request should be a json
array containing a list of objects. each object represents one row to be
inserted. you may therefore insert one or more rows per request.

each json object must have exactly two properties: "table" and "data".
the table property should contain a string with the name of the table into
which the row should be inserted and the "data" property should contain
another json object with the actual data to be inserted

###### Example:

        >> POST /api/v1/tables/insert HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> [
        >>   {
        >>     "table": "my_sensor_table",
        >>     "data": {
        >>       "time": "2015-10-11T17:53:03Z",
        >>       "sensor_name": "temperature_outside",
        >>       "sensor_value": 8.634
        >>     }
        >>   },
        >>   {
        >>     "table": "my_sensor_table",
        >>     "data": {
        >>       "time": "2015-10-11T17:53:04Z",
        >>       "sensor_name": "temperature_inside",
        >>       "sensor_value": 21.282
        >>     }
        >>   }
        >> ]

        << HTTP/1.1 201 CREATED
        << Content-Length: 0


---
### POST /api/v1/tables/create_table

create a new table (or overwrite an existing table)

      parameters (JSON):
          table_name: name of the table to be created/overwritten
          table_type: the type of the table to be created. must be "static" or "timeseries"
          schema: the json schema of the table
          update (optional): if true, overwrite an existing table if a table with the same name exists

      example

        >> POST /api/v1/tables/create_table HTTP/1.1
        >> Authorization: Token <authtoken>
        >> Content-Type: application/json
        >> Content-Length: ...
        >>
        >> {
        >>   "table_name": "my_sensor_table",
        >>   "table_type": "timeseries",
        >>   "schema": {
        >>      "columns": [
        >>          {
        >>             "id": 1,
        >>             "name": "time",
        >>             "type": "DATETIME",
        >>             "optional": "false"
        >>          },
        >>          {
        >>             "id": 2,
        >>             "name": "sensor_name",
        >>             "type": "STRING",
        >>             "optional": "false"
        >>          },
        >>          {
        >>             "id": 3,
        >>             "name": "sensor_value",
        >>             "type": "DOUBLE"
        >>             "optional": "true"
        >>          }
        >>      ]
        >>   }
        >> }

        << HTTP/1.1 201 CREATED
        << Content-Length: 0


## Session Tracking


---
### POST /api/v1/session_tracking/attributes/add_attribute

add a session tracking attribute

    parameters:
        name      -- the name of the attribute to add
        type      -- the type of the attribute to be added, must be one of OBJECT,
                     BOOLEAN, UINT32, STRING, UINT64, DOUBLE, DATETIME
        repeated  -- [optional] "true" if the new attribute should be a repeated attribute
        optional  -- [optional] "true" if the new attribute should be optional

---
### POST /api/v1/session_tracking/attributes/remove_attribute

remove a session tracking attribute

    parameters:
        name      -- the name of the attribute to remove


---
### POST /api/v1/session_tracking/events/add_event

add a session tracking event

    parameters:
        event     -- the name of the event to add

---
### POST /api/v1/session_tracking/events/remove_event

remove a session tracking event

    parameters:
        event     -- the name of the event to remove


---
### POST /api/v1/session_tracking/events/add_field

add a field to an existing session tracking event

    parameters:
        event     -- the name of the event to add the field to
        field     -- the name of the field to add
        type      -- the type of the field to be added, must be one of OBJECT,
                     BOOLEAN, UINT32, STRING, UINT64, DOUBLE, DATETIME
        repeated  -- [optional] "true" if the new field should be a repeated field
        optional  -- [optional] "true" if the new field should be optional

---
### POST /api/v1/session_tracking/events/remove_field

  remove a field from an existing session tracking event.

    parameters:
        event     -- the name of the event to remove the field from
        field     -- the name of the field to remove

    example:
        POST /api/v1/session_tracking/events/remove_field?event=search_query&field=result_items.shop_id



## Logfiles

---
### POST /api/v1/logfiles/add_logfile

add a logfile

  parameters:
      logfile    -- the name of the logfile to add


---
### POST /api/v1/logfiles/set_regex

update the regex of an existing logfile

    v1:
      params:
        logfile   -- the name of the logfile whose regex is set
        regex     -- the regex to set (uri encoded)

    v2:
      params:
        logfile   -- the name of the logfile whose regex is set

      postbody:
        regex     -- the regex to set


---
### POST /api/v1/logfiles/add_source_field

add a source field to an existing logfile

    parameters:
        logfile   -- the name of the logfile to add the source field to
        name      -- the name of the source field to add
        type      -- the type of the row field to be added, must be one of OBJECT,
                     BOOLEAN, UINT32, STRING, UINT64, DOUBLE, DATETIME 
        format    -- the format of the source field to add (only mandatory if type == DATETIME?)


---
### POST /api/v1/logfiles/add_row_field

add a row field to an existing logfile

    parameters:
        logfile   -- the name of the logfile to add the row field to
        name      -- the name of the row field to add
        type      -- the type of the row field to be added, must be one of OBJECT,
                     BOOLEAN, UINT32, STRING, UINT64, DOUBLE, DATETIME
        format    -- the format of the row field to add (only mandatory if type == DATETIME?)


---
### POST /api/v1/logfiles/remove_source_field

remove a source field from an existing logfile

    parameters:
        logfile   -- the name of the logfile to remove the source field from
        name      -- the name of the source field to remove


---
### POST /api/v1/logfiles/remove_row_field

remove a row field from an existing logfile

    parameters:
        logfile   -- the name of the logfile to remove the row field from
        name      -- the name of the row field to remove


