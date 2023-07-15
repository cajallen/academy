#include "process_launch.hpp"

#include <windows.h>

namespace spellbook {

void launch_subprocess(const string& application_name) {
    STARTUPINFO si = {};   
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    std::wstring temp = std::wstring(application_name.begin(), application_name.end());
    LPCWSTR application_name_wstr = temp.c_str();

    // start the program up
    CreateProcess(application_name.c_str(),
        nullptr,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
}

}