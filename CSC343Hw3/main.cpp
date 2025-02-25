#include <windows.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <climits>  
#include <string>

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0)
    {
        if (argc < 12)
        {
            std::cerr << "Error: Child process did not receive enough arguments." << std::endl;
            return 1;
        }
        int minVal = INT_MAX;
        for (int i = 2; i < 12; i++)
        {
            int num = std::atoi(argv[i]);
            if (num < minVal)
                minVal = num;
        }
        DWORD childPid = GetCurrentProcessId();
        std::cout << childPid << " " << minVal;
        return 0;
    }
    else // Parent process mode
    {
        const int ARRAY_SIZE = 20;
        int arr[ARRAY_SIZE];

        srand(static_cast<unsigned int>(time(0)));  // random values (0-99)
        std::cout << "Array: ";
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            arr[i] = rand() % 100;
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;

        int parentMin = INT_MAX;
        for (int i = 0; i < ARRAY_SIZE / 2; i++)
        {
            if (arr[i] < parentMin)
                parentMin = arr[i];
        }
        DWORD parentPid = GetCurrentProcessId();

        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;  
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;
        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
        {
            std::cerr << "Stdout pipe creation failed." << std::endl;
            return 1;
        }
        if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
        {
            std::cerr << "Stdout SetHandleInformation failed." << std::endl;
            return 1;
        }

        std::stringstream cmdLine;
        cmdLine << "\"" << argv[0] << "\" child";
        for (int i = ARRAY_SIZE / 2; i < ARRAY_SIZE; i++)
        {
            cmdLine << " " << arr[i];
        }
        std::string cmdLineStr = cmdLine.str();
        char* cmdLineCStr = new char[cmdLineStr.size() + 1];
        strcpy_s(cmdLineCStr, cmdLineStr.size() + 1, cmdLineStr.c_str());

        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        STARTUPINFOA siStartInfo;
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        BOOL bSuccess = CreateProcessA(NULL, cmdLineCStr, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);   
        delete[] cmdLineCStr; 

        if (!bSuccess)
        {
            std::cerr << "CreateProcess failed." << std::endl;
            return 1;
        }

        CloseHandle(hChildStd_OUT_Wr);

        DWORD dwRead;
        CHAR chBuf[128];
        std::string childOutput;
        while (ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL) && dwRead > 0)
        {
            chBuf[dwRead] = '\0';
            childOutput += chBuf;
        }
        CloseHandle(hChildStd_OUT_Rd);

        WaitForSingleObject(piProcInfo.hProcess, INFINITE);

        std::istringstream iss(childOutput);
        DWORD childPid;
        int childMin;
        iss >> childPid >> childMin;

        std::cout << "Parent Process (ID: " << parentPid << "): Minimum in first half = " << parentMin << std::endl;
        std::cout << "Child Process (ID: " << childPid << "): Minimum in second half = " << childMin << std::endl;
        int overallMin = (parentMin < childMin) ? parentMin : childMin;
        std::cout << "Overall minimum in the array = " << overallMin << std::endl;

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        return 0;
    }
}