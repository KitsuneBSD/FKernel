#pragma once 

class IInterruptHandler {
public:
    virtual ~IInterruptHandler() = default;
    virtual void Handle() = 0;
};