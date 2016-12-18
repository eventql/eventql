module EventQL; end

class EventQL::Client

  def initialize(opts = {})
    @opts = opts
  end

  def query(query_str, opts = {})
    EventQL::Query.new(self, query_str, opts)
  end

  def get_host
    return @opts[:host]
  end

  def get_port
    return @opts[:port]
  end

  def get_database
    return @opts[:database]
  end

  def get_auth_token
    return @opts[:auth_token]
  end

  def has_auth_token?
    return !@opts[:auth_token].nil?
  end

end
