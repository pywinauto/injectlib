#include <windows.h>
#include <thread>
#include "qt_server.h"

// Keep DllMain small and start server in Qt GUI thread
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Spawn a detached worker that will bootstrap on its own time.
        std::thread([] {
            QtHelloServer::bootstrap(); // waits for QCoreApplication, then queues start() on Qt thread
        }).detach();
        break;

    case DLL_PROCESS_DETACH:
        // Stop on the Qt thread
        QtHelloServer::shutdown();
        break;

    default:
        break;
    }
    return TRUE;
}

