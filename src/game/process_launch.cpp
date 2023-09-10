#include "process_launch.hpp"

#include <windows.h>

#include "general/string.hpp"

namespace spellbook {

void launch_subprocess(const FilePath& application_path) {
    STARTUPINFO si = {};   
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    // start the program up
    string abs_path = application_path.abs_string();
    CreateProcess(abs_path.c_str(),
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