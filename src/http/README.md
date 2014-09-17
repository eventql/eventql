


HTTP Core API
=============

- class types
  - `HeaderField` - represents an HTTP header field
  - `HeaderFieldList` - represents a set of headers
  - `HttpConnectionFactory` - base class for specific HTTP connection factories
  - `HttpDateGenerator` - API to generate the HTTP Date response header
  - `HttpInput` - HTTP request message body (consumer)
  - `HttpListener` - HTTP transport event listener interface
  - `HttpChannel : HttpListener` - Semantic HTTP layer
  - `HttpOutput` - HTTP response message body (builder)
  - `HttpRequest` - generic HTTP request
  - `HttpResponse` - generic HTTP response
  - `HttpTransport` - generic HTTP transport layer
- enum types
  - `HttpStatus` - HTTP message response status code
  - `HttpVersion` - HTTP protocol version


