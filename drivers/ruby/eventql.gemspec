Gem::Specification.new do |s|
  s.name = "eventql"
  s.version = "0.0.3"
  s.authors = ["Paul Asmuth"]
  s.date = "2016-12-18"
  s.description = "EventQL ruby driver"
  s.summary = "EventQL ruby driver"
  s.email = "paul@eventql.io"
  s.files = %w(
    eventql.gemspec
    lib/eventql_client.rb
    lib/eventql_query.rb
    lib/eventql.rb
  )
  s.homepage = "http://github.com/eventql/eventql"
  s.licenses = ["MIT"]
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.10"
end
