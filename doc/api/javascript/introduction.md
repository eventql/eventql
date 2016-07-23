3. Pipelines
============

To enable the most complex queries and data processing pipelines, EventQL allows you
to execute batch and stream processing jobs written in JavaScript.

When you execute your code on EventQL, it is automatically distributed and parallelized --
you can take your mind off worrying about where and when your code runs and focus
on building business logic. Once you have written a pipeline or query you will be
able to run it on it on 1GB, 100GB or 100TB of data without modifying a single
line

This guide will walk you through running a simple MapReduce Job. 
We will calculate the referrer stats of a fictional web shop

#### Create Table
For our example, we will create a table that stores `time`, `session_id` and `url` for
each page_view.

Run this from the command line (..against your local or remote server/cluster, database)

    $ evql -e "CREATE TABLE access_logs (time DATETIME PRIMARY KEY, session_id STRING, url STRING);"


#### Insert Data


    $ evql -e "INSERT INTO access_logs FROM JSON '{"time": "2016-07-23
  

#### 
Now, let


An example says more than a thousand words so here is a simple script that
calculates the bounce rate per page in the last 30 days from raw pageview data


    var pageviews_mapped = EVQL.mapTable({
      table: "web.pageviews",
      from: "-30d",
      until: "now",
      map_fn: function(row) {
        return [[row.url, row.bounced ? "did-bounce" : "did-not-bounce"]];
      }
    });

    var bouncerate_per_page = EVQL.reduce({
      sources: [pageviews_mapped],
      reduce_fn: function(url, visits) {
        var num_pageviews = 0;
        var num_bounces = 0;

        while (visits.hasNext()) {
          var visit = visits.next();
          num_pageviews += 1;
          num_bounces += (visit == "did-bounce") ? 1 : 0;
        }

        return [[url, num_bounces / num_pageviews]];
      }
    });

    EVQL.downloadResults(bouncerate_per_page);


### Getting Started with pipelines

You develop pipelines on your machine and run them from the command line
using the `evql` tool. Once you are happy with your code you can either continue
to run it locally or deploy it to your EVQL cluster.

To test-drive the pipelines feature, save this into a file called `helloworld.js`:

    console.log(1);

And then execute it by running:

      $ evql -f helloworld.js


To get started writing your own pipelines, have a look at these pages. If you
are already familiar with common data processing paradigms like MapReduce you
won't have to learn any new concepts: jump straight into the API documentation.

  - [Map Reduce](/docs/piplines/map_reduce)
  - [JavaScript API Documentation](/docs/pipelines/javascript_api)






