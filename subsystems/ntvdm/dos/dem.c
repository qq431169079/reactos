/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dem.c
 * PURPOSE:         DOS 32-bit Emulation Support Library -
 *                  This library is used by the built-in NTVDM DOS32 and by
 *                  the NT 16-bit DOS in Windows (via BOPs). It also exposes
 *                  exported functions that can be used by VDDs.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "utils.h"

#include "dem.h"
#include "cpu/bop.h"

#include "bios/bios.h"
#include "mouse32.h"

/* Extra PSDK/NDK Headers */
#include <ndk/obtypes.h>

/* PRIVATE VARIABLES **********************************************************/

/**/extern BYTE CurrentDrive;/**/

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_LOAD_DOS    0x2B    // DOS Loading and Initializing BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_START_DOS   0x2C    // DOS Starting BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_DOS         0x50    // DOS System BOP (for NTIO.SYS and NTDOS.SYS)
#define BOP_CMD         0x54    // DOS Command Interpreter BOP (for COMMAND.COM)

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI DosSystemBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case 0x11:  // Load the DOS kernel
        {
            BOOLEAN Success = FALSE;
            HANDLE  hDosKernel;
            ULONG   ulDosKernelSize = 0;

            DPRINT1("You are loading Windows NT DOS!\n");

            /* Open the DOS kernel file */
            hDosKernel = FileOpen("ntdos.sys", &ulDosKernelSize);

            /* If we failed, bail out */
            if (hDosKernel == NULL) goto Quit;

            /*
             * Attempt to load the DOS kernel into memory.
             * The segment where to load the DOS kernel is defined
             * by the DOS BIOS and is found in DI:0000 .
             */
            Success = FileLoadByHandle(hDosKernel,
                                       REAL_TO_PHYS(TO_LINEAR(getDI(), 0x0000)),
                                       ulDosKernelSize,
                                       &ulDosKernelSize);

            DPRINT1("Windows NT DOS loading %s at %04X:%04X, size 0x%X ; GetLastError() = %u\n",
                    (Success ? "succeeded" : "failed"),
                    getDI(), 0x0000,
                    ulDosKernelSize,
                    GetLastError());

            /* Close the DOS kernel file */
            FileClose(hDosKernel);

Quit:
            if (!Success)
            {
                /* We failed everything, stop the VDM */
                EmulatorTerminate();
            }

            break;
        }

        default:
        {
            DPRINT1("Unknown DOS System BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}

static VOID WINAPI DosCmdInterpreterBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case 0x08:  // Launch external command
        {
#define CMDLINE_LENGTH  1024

            BOOL Result;
            DWORD dwExitCode;

            LPSTR Command = (LPSTR)SEG_OFF_TO_PTR(getDS(), getSI());
            LPSTR CmdPtr  = Command;
            CHAR CommandLine[CMDLINE_LENGTH] = "";
            STARTUPINFOA StartupInfo;
            PROCESS_INFORMATION ProcessInformation;

            /* NULL-terminate the command line by removing the return carriage character */
            while (*CmdPtr && *CmdPtr != '\r') CmdPtr++;
            *CmdPtr = '\0';

            DPRINT1("CMD Run Command '%s'\n", Command);

            /* Spawn a user-defined 32-bit command preprocessor */

            /* Build the command line */
            // FIXME: Use COMSPEC env var!!
            strcpy(CommandLine, "cmd.exe /c ");
            strcat(CommandLine, Command);

            RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
            RtlZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

            StartupInfo.cb = sizeof(StartupInfo);

            VidBiosDetachFromConsole();

            Result = CreateProcessA(NULL,
                                    CommandLine,
                                    NULL,
                                    NULL,
                                    TRUE,
                                    0,
                                    NULL,
                                    NULL,
                                    &StartupInfo,
                                    &ProcessInformation);
            if (Result)
            {
                DPRINT1("Command '%s' launched successfully\n", Command);

                /* Wait for process termination */
                WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

                /* Get the exit code */
                GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);

                /* Close handles */
                CloseHandle(ProcessInformation.hThread);
                CloseHandle(ProcessInformation.hProcess);
            }
            else
            {
                DPRINT1("Failed when launched command '%s'\n");
                dwExitCode = GetLastError();
            }

            VidBiosAttachToConsole();

            setAL((UCHAR)dwExitCode);

            break;
        }

        default:
        {
            DPRINT1("Unknown DOS CMD Interpreter BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}

#ifndef STANDALONE
static DWORD
WINAPI
CommandThreadProc(LPVOID Parameter)
{
    BOOLEAN First = TRUE;
    DWORD Result;
    VDM_COMMAND_INFO CommandInfo;
    CHAR CmdLine[MAX_PATH];
    CHAR AppName[MAX_PATH];
    CHAR PifFile[MAX_PATH];
    CHAR Desktop[MAX_PATH];
    CHAR Title[MAX_PATH];
    ULONG EnvSize = 256;
    PVOID Env = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, EnvSize);

    UNREFERENCED_PARAMETER(Parameter);
    ASSERT(Env != NULL);

    do
    {
        /* Clear the structure */
        RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

        /* Initialize the structure members */
        CommandInfo.TaskId = SessionId;
        CommandInfo.VDMState = VDM_FLAG_DOS;
        CommandInfo.CmdLine = CmdLine;
        CommandInfo.CmdLen = sizeof(CmdLine);
        CommandInfo.AppName = AppName;
        CommandInfo.AppLen = sizeof(AppName);
        CommandInfo.PifFile = PifFile;
        CommandInfo.PifLen = sizeof(PifFile);
        CommandInfo.Desktop = Desktop;
        CommandInfo.DesktopLen = sizeof(Desktop);
        CommandInfo.Title = Title;
        CommandInfo.TitleLen = sizeof(Title);
        CommandInfo.Env = Env;
        CommandInfo.EnvLen = EnvSize;

        if (First) CommandInfo.VDMState |= VDM_FLAG_FIRST_TASK;

Command:
        if (!GetNextVDMCommand(&CommandInfo))
        {
            if (CommandInfo.EnvLen > EnvSize)
            {
                /* Expand the environment size */
                EnvSize = CommandInfo.EnvLen;
                CommandInfo.Env = Env = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Env, EnvSize);

                /* Repeat the request */
                goto Command;
            }

            break;
        }

        /* Start the process from the command line */
        DPRINT1("Starting '%s' ('%s')...\n", AppName, CmdLine);
        Result = DosStartProcess(AppName, CmdLine, Env);
        if (Result != ERROR_SUCCESS)
        {
            DisplayMessage(L"Could not start '%S'. Error: %u", AppName, Result);
            // break;
            continue;
        }

        First = FALSE;
    }
    while (AcceptCommands);

    HeapFree(GetProcessHeap(), 0, Env);
    return 0;
}
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

//
// This function (equivalent of the DOS bootsector) is called by the bootstrap
// loader *BEFORE* jumping at 0000:7C00. What we should do is to write at 0000:7C00
// a BOP call that calls DosInitialize back. Then the bootstrap loader jumps at
// 0000:7C00, our BOP gets called and then we can initialize the 32-bit part of the DOS.
//

/* 16-bit bootstrap code at 0000:7C00 */
/* Of course, this is not in real bootsector format, because we don't care */
static BYTE Bootsector1[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_LOAD_DOS,  // Call DOS Loading
};
/* This portion of code is run if we failed to load the DOS */
static BYTE Bootsector2[] =
{
    0xEA,                   // jmp far ptr
    0x5B, 0xE0, 0x00, 0xF0, // F000:E05B /** HACK! What to do instead?? **/
};

static VOID WINAPI DosInitialize(LPWORD Stack);

VOID DosBootsectorInitialize(VOID)
{
    /* We write the bootsector at 0000:7C00 */
    ULONG_PTR Address = (ULONG_PTR)SEG_OFF_TO_PTR(0x0000, 0x7C00);
    CHAR DosKernelFileName[] = ""; // No DOS file name, therefore we'll load DOS32

    DPRINT("DosBootsectorInitialize\n");

    /* Write the "bootsector" */
    RtlCopyMemory((PVOID)Address, Bootsector1, sizeof(Bootsector1));
    Address += sizeof(Bootsector1);
    RtlCopyMemory((PVOID)Address, DosKernelFileName, sizeof(DosKernelFileName));
    Address += sizeof(DosKernelFileName);
    RtlCopyMemory((PVOID)Address, Bootsector2, sizeof(Bootsector2));

    /* Register the DOS Loading BOP */
    RegisterBop(BOP_LOAD_DOS, DosInitialize);
}


//
// This function is called by the DOS bootsector. We finish to load
// the DOS, then we jump to 0070:0000.
//

/* 16-bit startup code at 0070:0000 */
static BYTE Startup[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_START_DOS,  // Call DOS Start
};

static VOID WINAPI DosStart(LPWORD Stack);

static VOID WINAPI DosInitialize(LPWORD Stack)
{
    BOOLEAN Success = FALSE;

    /* Get the DOS kernel file name (NULL-terminated) */
    // FIXME: Isn't it possible to use some DS:SI instead??
    LPCSTR DosKernelFileName = (LPCSTR)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + strlen(DosKernelFileName) + 1); // Skip it

    DPRINT("DosInitialize('%s')\n", DosKernelFileName);

    /* Register the DOS BOPs */
    RegisterBop(BOP_DOS, DosSystemBop        );
    RegisterBop(BOP_CMD, DosCmdInterpreterBop);

    if (DosKernelFileName && DosKernelFileName[0] != '\0')
    {
        HANDLE  hDosBios;
        ULONG   ulDosBiosSize = 0;

        /* Open the DOS BIOS file */
        hDosBios = FileOpen(DosKernelFileName, &ulDosBiosSize);

        /* If we failed, bail out */
        if (hDosBios == NULL) goto QuitCustom;

        /* Attempt to load the DOS BIOS into memory */
        Success = FileLoadByHandle(hDosBios,
                                   REAL_TO_PHYS(TO_LINEAR(0x0070, 0x0000)),
                                   ulDosBiosSize,
                                   &ulDosBiosSize);

        DPRINT1("DOS BIOS loading %s at %04X:%04X, size 0x%X ; GetLastError() = %u\n",
                (Success ? "succeeded" : "failed"),
                0x0070, 0x0000,
                ulDosBiosSize,
                GetLastError());

        /* Close the DOS BIOS file */
        FileClose(hDosBios);

        if (!Success) goto QuitCustom;

        /* Position execution pointers and return */
        setCS(0x0070);
        setIP(0x0000);

        /* Return control */
QuitCustom:
        if (!Success)
            DisplayMessage(L"Custom DOS '%S' loading failed, what to do??", DosKernelFileName);
    }
    else
    {
        Success = DosBIOSInitialize();
        // Success &= DosKRNLInitialize();

        if (!Success) goto Quit32;

        /* Write the "bootsector" */
        RtlCopyMemory(SEG_OFF_TO_PTR(0x0070, 0x0000), Startup, sizeof(Startup));

        /* Register the DOS Starting BOP */
        RegisterBop(BOP_START_DOS, DosStart);

        /* Position execution pointers and return */
        setCS(0x0070);
        setIP(0x0000);

        /* Return control */
Quit32:
        if (!Success)
            DisplayMessage(L"DOS32 loading failed, what to do??");
    }

    if (Success)
    {
        /*
         * We succeeded, deregister the DOS Loading BOP
         * so that no app will be able to call us back.
         */
        RegisterBop(BOP_LOAD_DOS, NULL);
    }
}

static VOID WINAPI DosStart(LPWORD Stack)
{
#ifdef STANDALONE
    DWORD Result;
    CHAR ApplicationName[MAX_PATH];
    CHAR CommandLine[DOS_CMDLINE_LENGTH];
#endif

    DPRINT("DosStart\n");

    /*
     * We succeeded, deregister the DOS Starting BOP
     * so that no app will be able to call us back.
     */
    RegisterBop(BOP_START_DOS, NULL);

    /* Load the mouse driver */
    // DosMouseInitialize();

#ifndef STANDALONE

    /* Create the GetNextVDMCommand thread */
    CommandThread = CreateThread(NULL, 0, &CommandThreadProc, NULL, 0, NULL);
    if (CommandThread == NULL)
    {
        wprintf(L"FATAL: Failed to create the command processing thread: %d\n", GetLastError());
        goto Quit;
    }

    /* Wait for the command thread to exit */
    WaitForSingleObject(CommandThread, INFINITE);

    /* Close the thread handle */
    CloseHandle(CommandThread);

#else

    if (NtVdmArgc >= 2)
    {
        WideCharToMultiByte(CP_ACP, 0, NtVdmArgv[1], -1, ApplicationName, sizeof(ApplicationName), NULL, NULL);

        if (NtVdmArgc >= 3)
            WideCharToMultiByte(CP_ACP, 0, NtVdmArgv[2], -1, CommandLine, sizeof(CommandLine), NULL, NULL);
        else
            strcpy(CommandLine, "");
    }
    else
    {
        DisplayMessage(L"Invalid DOS command line\n");
        goto Quit;
    }

    /* Start the process from the command line */
    DPRINT1("Starting '%s' ('%s')...\n", ApplicationName, CommandLine);
    Result = DosStartProcess(ApplicationName,
                             CommandLine,
                             SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0));
    if (Result != ERROR_SUCCESS)
    {
        DisplayMessage(L"Could not start '%S'. Error: %u", ApplicationName, Result);
        goto Quit;
    }

#endif

Quit:
    /* Stop the VDM */
    EmulatorTerminate();
}

/* PUBLIC EXPORTED APIS *******************************************************/

// demLFNCleanup
// demLFNGetCurrentDirectory

// demGetFileTimeByHandle_WOW
// demWOWLFNAllocateSearchHandle
// demWOWLFNCloseSearchHandle
// demWOWLFNEntry
// demWOWLFNGetSearchHandle
// demWOWLFNInit

DWORD
WINAPI
demClientErrorEx(IN HANDLE FileHandle,
                 IN CHAR   Unknown,
                 IN BOOL   Flag)
{
    UNIMPLEMENTED;
    return GetLastError();
}

DWORD
WINAPI
demFileDelete(IN LPCSTR FileName)
{
    if (DeleteFileA(FileName)) SetLastError(ERROR_SUCCESS);

    return GetLastError();
}

DWORD
WINAPI
demFileFindFirst(OUT PVOID  lpFindFileData,
                 IN  LPCSTR FileName,
                 IN  WORD   AttribMask)
{
    BOOLEAN Success = TRUE;
    WIN32_FIND_DATAA FindData;
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

    /* Fill the block */
    FindFileBlock->DriveLetter  = CurrentDrive + 'A';
    FindFileBlock->AttribMask   = AttribMask;
    FindFileBlock->SearchHandle = FindFirstFileA(FileName, &FindData);
    if (FindFileBlock->SearchHandle == INVALID_HANDLE_VALUE) return GetLastError();

    do
    {
        /* Check the attributes */
        if (!((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                            FILE_ATTRIBUTE_SYSTEM |
                                            FILE_ATTRIBUTE_DIRECTORY))
            & ~AttribMask))
        {
            break;
        }
    }
    while ((Success = FindNextFileA(FindFileBlock->SearchHandle, &FindData)));

    if (!Success) return GetLastError();

    FindFileBlock->Attributes = LOBYTE(FindData.dwFileAttributes);
    FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                          &FindFileBlock->FileDate,
                          &FindFileBlock->FileTime);
    FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                     : FindData.nFileSizeLow;
    strcpy(FindFileBlock->FileName, FindData.cAlternateFileName);

    return ERROR_SUCCESS;
}

DWORD
WINAPI
demFileFindNext(OUT PVOID lpFindFileData)
{
    WIN32_FIND_DATAA FindData;
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

    do
    {
        if (!FindNextFileA(FindFileBlock->SearchHandle, &FindData))
            return GetLastError();

        /* Update the block */
        FindFileBlock->Attributes = LOBYTE(FindData.dwFileAttributes);
        FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                              &FindFileBlock->FileDate,
                              &FindFileBlock->FileTime);
        FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                         : FindData.nFileSizeLow;
        strcpy(FindFileBlock->FileName, FindData.cAlternateFileName);
    }
    while((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                        FILE_ATTRIBUTE_SYSTEM |
                                        FILE_ATTRIBUTE_DIRECTORY))
          & ~FindFileBlock->AttribMask);

    return ERROR_SUCCESS;
}

UCHAR
WINAPI
demGetPhysicalDriveType(IN UCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return DOSDEVICE_DRIVE_UNKNOWN;
}

BOOL
WINAPI
demIsShortPathName(IN LPCSTR Path,
                   IN BOOL Unknown)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
WINAPI
demSetCurrentDirectoryGetDrive(IN  LPCSTR CurrentDirectory,
                               OUT PUCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

/* EOF */
