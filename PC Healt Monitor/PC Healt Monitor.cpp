#include <windows.h>
#include <iostream>
#include <iomanip>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

using namespace std;


/* ================= CPU TEMPERATURE ================= */
double GetCPUTemperature()
{
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return -1;

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres) && hres != RPC_E_TOO_LATE)
        return -1;

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc
    );

    if (FAILED(hres)) return -1;

    IWbemServices* pSvc = NULL;

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\WMI"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres))
    {
        pLoc->Release();
        return -1;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    IEnumWbemClassObject* pEnumerator = NULL;

    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        return -1;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    double temperature = -1;

    if (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (uReturn != 0)
        {
            VARIANT vtProp;

            hr = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);

            if (SUCCEEDED(hr))
            {
                double tempKelvin = vtProp.uintVal / 10.0;
                temperature = tempKelvin - 273.15;

                VariantClear(&vtProp);
            }

            pclsObj->Release();
        }

        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return temperature;
}


/* ================= CPU USAGE ================= */

double GetCPUUsage()
{
    static ULONGLONG lastIdle = 0;
    static ULONGLONG lastKernel = 0;
    static ULONGLONG lastUser = 0;

    FILETIME idleTime, kernelTime, userTime;

    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    ULONGLONG idle =
        ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;

    ULONGLONG kernel =
        ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;

    ULONGLONG user =
        ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    ULONGLONG idleDiff = idle - lastIdle;
    ULONGLONG kernelDiff = kernel - lastKernel;
    ULONGLONG userDiff = user - lastUser;

    ULONGLONG total = kernelDiff + userDiff;

    lastIdle = idle;
    lastKernel = kernel;
    lastUser = user;

    if (total == 0) return 0.0;

    return (1.0 - (double)idleDiff / total) * 100.0;
}

/* ================= RAM USAGE ================= */

double GetRAMUsage()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);

    GlobalMemoryStatusEx(&memInfo);

    double used =
        (double)(memInfo.ullTotalPhys - memInfo.ullAvailPhys);

    double total =
        (double)memInfo.ullTotalPhys;

    return (used / total) * 100.0;
}

/* ================= UI ================= */

void DrawUI(double cpu, double ram, double temp)
{
    system("cls");

    cout << "=============================\n";
    cout << "     PC HEALTH MONITOR\n";
    cout << "=============================\n\n";

    cout << fixed << setprecision(1);

    cout << "Thanks for using the app!\n";
    cout << "CPU Usage : " << cpu << " %\n";
    cout << "RAM Usage : " << ram << " %\n\n";

    if (cpu > 85)
        cout << "> WARNING: High CPU Usage!\n";

    if (ram > 85)
        cout << "> WARNING: High RAM Usage!\n";

    if (temp > 0)
        cout << "CPU Temp  : " << temp << " °C\n";
    else
        cout << "CPU Temp  : Not Available\n";


    cout << "\nPress CTRL + C to Exit\n";
}

/* ================= MAIN ================= */

int main()
{
    cout << "Starting PC Health Monitor...\n";
    cout << "Please wait while PC Health Monitor getting ready for you..." << endl;
    Sleep(1000);

    while (true)
    {
        double cpu = GetCPUUsage();
        double ram = GetRAMUsage();
        double temp = GetCPUTemperature();

        DrawUI(cpu, ram, temp);

        Sleep(2000);
    }
}