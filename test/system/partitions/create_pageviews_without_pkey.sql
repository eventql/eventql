CREATE TABLE pageviews_without_pkey (
  `time` datetime,
  `sid` uint64,
  `request_id` uint64,
  `url` string,
  `user_agent` string,
  `referrer` string,
  `product_id` uint64,
  `time_on_page` double,
  `screen_width` uint64,
  `screen_height` uint64,
  `is_logged_in` bool,
  PRIMARY KEY (time, sid, request_id)
);
