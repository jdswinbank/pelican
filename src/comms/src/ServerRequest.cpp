#include "ServerRequest.h"
#include "utility/memCheck.h"


namespace pelican {


// class ServerRequest 
ServerRequest::ServerRequest(Request type, const QString& msg)
    : _type(type), _error(msg)
{
}

ServerRequest::~ServerRequest()
{
}

ServerRequest::Request ServerRequest::type() const
{
    return _type;
}

void ServerRequest::error(const QString& msg)
{
    _error = msg;
    _type = Error;
}

QString ServerRequest::message() const
{
    return _error;
}

bool ServerRequest::operator==(const ServerRequest& req) const
{
    return _type == req._type;
}

} // namespace pelican
