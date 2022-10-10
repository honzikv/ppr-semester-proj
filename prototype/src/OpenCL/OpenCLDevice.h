#pragma once

#include <mutex>

class OpenCLDevice {

public:
    void SetUsed(bool used) {
        this->used = used;
    }

private:
    bool used = false; // Flag for checking if the device is currently in use

};
