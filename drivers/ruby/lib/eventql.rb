require "eventql_client"
require "eventql_query"

module EventQL

  def self.connect(opts = {})
    return EventQL::Client.new(opts)
  end

end
