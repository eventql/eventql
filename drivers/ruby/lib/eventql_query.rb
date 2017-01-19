module EventQL; end

require "net/http"
require "json"

class EventQL::Query

  def initialize(client, query_str, query_opts = {})
    @client = client
    @query_str = query_str
    @query_opts = query_opts
  end

  def execute!
    request = Net::HTTP::Post.new("/api/v1/sql")
    request.add_field("Content-Type", "application/json")

    if @client.has_auth_token?
      request.add_field("Authorization", "Token #{@client.get_auth_token}")
    end

    request.body = {
      :query => @query_str,
      :database => @client.get_database,
      :format => "json"
    }.to_json

    http = Net::HTTP.new(@client.get_host, @client.get_port)
    response = http.request(request)

    response_json = JSON.parse(response.body) rescue nil
    if response_json && response_json.has_key?("error")
      raise response_json["error"]
    end

    if response_json.nil? || response.code.to_i != 200
      raise "HTTP ERROR (#{response.code}): #{response.body[0..128]}"
    end

    return response_json["results"]
  end

end
