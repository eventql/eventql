// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpStatus.h>

namespace xzero {

#define SRET(slit) { static std::string val(slit); return val; }

const std::string& to_string(HttpStatus code) {
  switch (code) {
    case HttpStatus::ContinueRequest: SRET("Continue Request");
    case HttpStatus::SwitchingProtocols: SRET("Switching Protocols");
    case HttpStatus::Processing: SRET("Processing");
    case HttpStatus::Ok: SRET("Ok");
    case HttpStatus::Created: SRET("Created");
    case HttpStatus::Accepted: SRET("Accepted");
    case HttpStatus::NonAuthoriativeInformation: SRET("Non Authoriative Information");
    case HttpStatus::NoContent: SRET("No Content");
    case HttpStatus::ResetContent: SRET("Reset Content");
    case HttpStatus::PartialContent: SRET("Partial Content");
    case HttpStatus::MultipleChoices: SRET("Multiple Choices");
    case HttpStatus::MovedPermanently: SRET("Moved Permanently");
    case HttpStatus::Found: SRET("Found");
    case HttpStatus::NotModified: SRET("Not Modified");
    case HttpStatus::TemporaryRedirect: SRET("Temporary Redirect");
    case HttpStatus::PermanentRedirect: SRET("Permanent Redirect");
    case HttpStatus::BadRequest: SRET("Bad Request");
    case HttpStatus::Unauthorized: SRET("Unauthorized");
    case HttpStatus::PaymentRequired: SRET("Payment Required");
    case HttpStatus::Forbidden: SRET("Forbidden");
    case HttpStatus::NotFound: SRET("Not Found");
    case HttpStatus::MethodNotAllowed: SRET("Method Not Allowed");
    case HttpStatus::NotAcceptable: SRET("Not Acceptable");
    case HttpStatus::ProxyAuthenticationRequired: SRET("Proxy Authentication Required");
    case HttpStatus::RequestTimeout: SRET("Request Timeout");
    case HttpStatus::Conflict: SRET("Conflict");
    case HttpStatus::Gone: SRET("Gone");
    case HttpStatus::LengthRequired: SRET("Length Required");
    case HttpStatus::PreconditionFailed: SRET("Precondition Failed");
    case HttpStatus::PayloadTooLarge: SRET("Payload Too Large");
    case HttpStatus::RequestUriTooLong: SRET("Request Uri Too Long");
    case HttpStatus::UnsupportedMediaType: SRET("Unsupported Media Type");
    case HttpStatus::RequestedRangeNotSatisfiable: SRET("Requested Range Not Satisfiable");
    case HttpStatus::ExpectationFailed: SRET("Expectation Failed");
    case HttpStatus::ThereAreTooManyConnectionsFromYourIP: SRET("There Are Too Many Connections From Your IP");
    case HttpStatus::UnprocessableEntity: SRET("Unprocessable Entity");
    case HttpStatus::Locked: SRET("Locked");
    case HttpStatus::FailedDependency: SRET("Failed Dependency");
    case HttpStatus::UnorderedCollection: SRET("Unordered Collection");
    case HttpStatus::UpgradeRequired: SRET("Upgrade Required");
    case HttpStatus::PreconditionRequired: SRET("Precondition Required");
    case HttpStatus::TooManyRequests: SRET("Too Many Requests");
    case HttpStatus::RequestHeaderFieldsTooLarge: SRET("Request Header Fields Too Large");
    case HttpStatus::InternalServerError: SRET("Internal Server Error");
    case HttpStatus::NotImplemented: SRET("Not Implemented");
    case HttpStatus::BadGateway: SRET("Bad Gateway");
    case HttpStatus::ServiceUnavailable: SRET("Service Unavailable");
    case HttpStatus::GatewayTimeout: SRET("Gateway Timeout");
    case HttpStatus::HttpVersionNotSupported: SRET("Http Version Not Supported");
    case HttpStatus::VariantAlsoNegotiates: SRET("Variant Also Negotiates");
    case HttpStatus::InsufficientStorage: SRET("Insufficient Storage");
    case HttpStatus::LoopDetected: SRET("Loop Detected");
    case HttpStatus::BandwidthExceeded: SRET("Bandwidth Exceeded");
    case HttpStatus::NotExtended: SRET("Not Extended");
    case HttpStatus::NetworkAuthenticationRequired: SRET("Network Authentication Required");
    default: SRET("");
  }
}

} // namespace xzero
