#include <switch.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
		// Initialize FS
    fsInitialize();
    fsdevMountSdmc();

    // Open file for writing
    FILE *f = fopen("sdmc:/hello.txt", "w");
    if (f) {
        fprintf(f, "hello world\n");
        fclose(f);
    }

    // Cleanup
    fsdevUnmountAll();
    fsExit();

    // Exit sysmodule
    return 0;
}
