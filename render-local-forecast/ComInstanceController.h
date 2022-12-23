#pragma once
#include <combaseapi.h>

class ComInstanceController {
private:
    HRESULT hr;

public:
    ComInstanceController() { hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); }
    bool Initalized() { return SUCCEEDED(hr); }
    ~ComInstanceController() { CoUninitialize(); }
};
