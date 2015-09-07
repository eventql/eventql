#Experiments API

The Experiments API allows you to tag traffic (i.e. user sessions, search queries or
result items) as being part of an “experiment”

###Basic Usage

Anywhere in your page, before including the DeepCortex tracking pixel, define the cmxdata
object:

```
var cmxdata = {
  // ...
};
```

If the current page view is part of an experiment, include the additional experiments property:

```
var cmxdata = {
  “experiments”: [
    { "key" => "my_user_based_experiment", "type" => "user" },
    { "key" => "my_query_based_experiment", "type" => "search" }
  ]
};
```

You can define an arbitrary number of experiments per page view

###Search experiments

Search experiments should be used if you want to analyze a change in your internal search
engine. The experiments property must be set on any search query result page that is
modified by the experiment.
A simple example:
Let’s suppose you are testing a new ranking algorithm called ‘mysuperanker’. You serve
50% of your users with the old algorithm, and 50% of your users with the new algorithm.
All you need to do in this case would be to include this snipped on every search result page
that was created using the new algorithm (and _not_ include it on search result pages that
are still using the old algorithm:

```
var cmxdata = {
  “experiments”: [
    { "key" => "‘mysuperanker’", "type" => "search" }
  ]
};
```

###User experiments

Search experiments should be used if you want to analyze an experiment in which you
modify some content or behaviour on a per-user basis.
A simple example:
Let’s suppose you are testing a new “add to cart button” color. 50% of users will see the old
color, 50% of users will see the new color on a session basis. I.e. once a user has seen
either the new or old color, they will always see that color.
All you need to do in this case would be to include this snipped on every page that shows
the new color:

```
var cmxdata = {
  “experiments”: [
    { "key" => "mynewbuttoncolor", "type" => "user" }
  ]
};
```
