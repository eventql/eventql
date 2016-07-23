5.1 MapReduce
=============

To enable the most complex queries and data processing pipelines, EventQL allows you
to execute batch and stream processing jobs written in JavaScript.

When you execute your code on EventQL, it is automatically distributed and parallelized --
you can take your mind off worrying about where and when your code runs and focus
on building business logic. Once you have written a pipeline or query you will be
able to run it on it on 1GB, 100GB or 100TB of data without modifying a single
line

### Example

An example says more than a thousand words so here is a simple example that will
calculate the top url stats from the raw pageview data of our access_log table, that
we created in [First Steps](../../getting-started/first-steps).

    var logs_mapped = EVQL.mapTable({
      table: "access_logs",
      required_columns: [
          "time",
          "url"
      ],
      map_fn: function(row) {
        return [[row.url, 1]];
      }
    });

    var top_urls = EVQL.reduce({
      sources: [logs_mapped],
      shards: 1,
      reduce_fn: function(url, visits) {
        var num_views = 0;
        while (visits.hasNext()) {
          num_views += 1;
        }

        return [[url, num_views]];
      }
    });

    EVQL.downloadResults([top_urls]);




### Getting Started with MapReduce scripts

You develop scripts on your local machine and run them from the command line
using the `evql` tool. Once you are happy with your code you can either continue
to run it locally or deploy it to your EVQL cluster.

To test-drive the pipelines feature, save this into a file called `helloworld.js`:

    console.log(1);

And then execute it by running:

    $ evql -f helloworld.js


To get started writing your own pipelines, have a look at these pages. If you
are already familiar with common data processing paradigms like MapReduce you
won't have to learn any new concepts: jump straight into the API documentation.

  - [JavaScript API Documentation](/docs/api/javascript_api)






