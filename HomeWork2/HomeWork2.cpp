#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <list>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <Windows.h>
#include "semaphore"
#include <stop_token>
#include <barrier>

using namespace std;

static string BIRDS_FILENAME = "Birds.txt";
static string DLL_NAME = "ICS0025DataProducerDll_C.dll";
static string DLL_FUNCTION_NAME = "DataProducer";

// Define the Control struct as provided in the task
struct Control
{
    atomic<int> nItems = 0;
    stop_token stop;
    condition_variable_any cva;
    binary_semaphore sm{ 0 };
    list<string> items;
};

static void* GetDataGenerator(HMODULE& DllHandle) {
    FARPROC pDataProducer;
    DllHandle = LoadLibraryA(DLL_NAME.c_str());
    if (!DllHandle) {
        cout << format("{} not found, error {}", DLL_NAME, GetLastError()) << endl;
        return nullptr;
    }

    pDataProducer = GetProcAddress(DllHandle, DLL_FUNCTION_NAME.c_str());
    if (pDataProducer == NULL) {
        FreeLibrary(DllHandle);
        cout << format("Function {} not found, error {}", DLL_FUNCTION_NAME, GetLastError()) << endl;
        return nullptr;
    }

    return pDataProducer;

}
int main() {
   
    HMODULE DllHandle;
    void(*ItemGenerator)(string, Control*) = (void(*)(string, Control*)) GetDataGenerator(DllHandle);

    if (ItemGenerator == NULL) {
        DWORD error = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL); // Use FormatMessageA
        cerr << "Failed to load DLL. Error code: " << error << ", Message: " << (LPSTR)lpMsgBuf << endl; // Use LPSTR
        LocalFree(lpMsgBuf);
        return 1;
    }

    // Initialize the Control struct
    Control* control = new Control();
    

    stop_source source; // to control the interrupting
     control->stop = source.get_token();
    
    // Call the function from the DLL
    jthread* producer = new jthread(ItemGenerator, BIRDS_FILENAME, control);

   //this_<thread_name>::sleep_for(chorno:: time_duration (time_period)) 
   this_thread::sleep_for(chrono::milliseconds(200));

    // Main command loop
    string command;
    bool loop = true;
    mutex mtx; // Local mutex for synchronization
    while (loop == true) {

        cout << "Enter command: " << endl;
        cin >> command;

        if (command == "exit") {
            
            loop = false;
        }
        else if (command == "print") {
            cout << "waiting for the adding to finish to print" << endl;
            control->sm.acquire();

            cout << "\nItems in data structure:" << endl; 
            for (const auto& item : control->items) {
                cout << item << endl;
            }
        }
        else if (command[0] == '#') {
            try {
                int n = stoi(command.substr(1));
                if (1 > n || n > 9)
                {
                    cout << "Invalid command" << endl;
                    continue;
                }
                control->nItems = n;
                control->cva.notify_all();
                
            }
            catch (const exception& e) {
                cerr << "Invalid command: " << e.what() << endl;
            }
        }
        else {
            cout << "Invalid command." << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    // Unload the DLL
    FreeLibrary(DllHandle);

    return 0;
}
