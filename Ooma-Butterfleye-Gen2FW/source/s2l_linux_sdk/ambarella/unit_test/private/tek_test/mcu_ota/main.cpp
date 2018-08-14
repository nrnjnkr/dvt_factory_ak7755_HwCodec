#include <iostream>
#include "efm32bootloader_client.h"
#include "qbytearray.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <uart_path> <file_path>" << endl;
        return 1;
    }

    EFM32BootloaderClient c(argv[1]);
    c.request_send(argv[2]);

    return 0;
}
