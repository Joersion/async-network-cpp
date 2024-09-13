#pragma once
#include "Session.h"
class SessionFactory {
public:
    SessionFactory();
    virtual ~SessionFactory();

    virtual std::shared_ptr<Session> create(boost::asio::io_context& ioContext, int timeout = 0) = 0;
};
