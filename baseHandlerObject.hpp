#pragma once

#include "fd.hpp"

#include <string>

class BaseHandlerObject {
protected:
    BaseHandlerObject() = default;
    BaseHandlerObject(const BaseHandlerObject &other) = default;
    BaseHandlerObject &operator=(const BaseHandlerObject &other) = default;
    virtual ~BaseHandlerObject() = default;

public:
    virtual void handleReadCallback(FD &fd, int funcReturnValue) = 0;
    virtual void handleWriteCallback(FD &fd) = 0;
    virtual void handleDisconnectCallback(FD &fd) = 0;
};
