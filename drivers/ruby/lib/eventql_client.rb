module EventQL; end

class EventQL::Client

  def initialize(opts = {})
    @opts = opts
  end

  def query(query_str, opts = {})
    EventQL::Query.new(self, query_str, opts)
  end

  def insert!(data, opts = {})
    request = Net::HTTP::Post.new("/api/v1/tables/insert")
    request.add_field("Content-Type", "application/json")

    if has_auth_token?
      request.add_field("Authorization", "Token #{get_auth_token}")
    end

    request.body = data.to_json
    puts request.body

    http = Net::HTTP.new(get_host, get_port)
    response = http.request(request)

    if response.code.to_i == 201
      return true
    else
      raise "HTTP ERROR (#{response.code}): #{response.body[0..128]}"
    end
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
