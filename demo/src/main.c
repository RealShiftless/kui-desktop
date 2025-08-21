#include <stddef.h>
#include "kui/kui.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInst;
    (void)hPrevInst;
    (void)lpCmdLine;
    (void)nCmdShow;
#else
int main(void) {
#endif
    kui_init((KuiArgs){
        .title  = "Kui Demo",
        .width  = 900,
        .height = 600 
    });

    kui_run();
    

    return 0;
}
