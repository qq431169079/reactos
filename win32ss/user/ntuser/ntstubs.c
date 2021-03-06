/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Native User stubs
 * FILE:             win32ss/user/ntuser/ntstubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

DWORD
APIENTRY
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB
    return 0;
}

//
// Works like BitBlt, http://msdn.microsoft.com/en-us/library/ms532278(VS.85).aspx
//
BOOL
APIENTRY
NtUserBitBltSysBmp(
   HDC hdc,
   INT nXDest,
   INT nYDest,
   INT nWidth,
   INT nHeight,
   INT nXSrc,
   INT nYSrc,
   DWORD dwRop )
{
   BOOL Ret = FALSE;
   UserEnterExclusive();

   Ret = NtGdiBitBlt( hdc,
                   nXDest,
                   nYDest,
                   nWidth,
                  nHeight,
                hSystemBM,
                    nXSrc,
                    nYSrc,
                    dwRop,
                        0,
                        0);

   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1
)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserDrawAnimatedRects(
   HWND hwnd,
   INT idAni,
   RECT *lprcFrom,
   RECT *lprcTo)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserEvent(
   DWORD Unknown0)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPWSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi)
{
   STUB

   return 0;
}

NTSTATUS
APIENTRY
NtUserInitializeClientPfnArrays(
  PPFNCLIENT pfnClientA,
  PPFNCLIENT pfnClientW,
  PPFNCLIENTWORKER pfnClientWorker,
  HINSTANCE hmodUser)
{
   NTSTATUS Status = STATUS_SUCCESS;
   TRACE("Enter NtUserInitializeClientPfnArrays User32 0x%p\n", hmodUser);

   if (ClientPfnInit) return Status;

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead( pfnClientA, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientW, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientWorker, sizeof(PFNCLIENTWORKER), 1);
      RtlCopyMemory(&gpsi->apfnClientA, pfnClientA, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientW, pfnClientW, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientWorker, pfnClientWorker, sizeof(PFNCLIENTWORKER));

      //// FIXME: HAX! Temporary until server side is finished.
      //// Copy the client side procs for now.
      RtlCopyMemory(&gpsi->aStoCidPfn, pfnClientW, sizeof(gpsi->aStoCidPfn));

      hModClient = hmodUser;
      ClientPfnInit = TRUE;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status =_SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      ERR("Failed reading Client Pfns from user space.\n");
      SetLastNtError(Status);
   }

   UserLeave();
   return Status;
}

DWORD
APIENTRY
NtUserInitTask(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5,
   DWORD Unknown6,
   DWORD Unknown7,
   DWORD Unknown8,
   DWORD Unknown9,
   DWORD Unknown10,
   DWORD Unknown11)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserMNDragLeave(VOID)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserMNDragOver(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserModifyUserStartupInfoFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserQueryUserCounters(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserRegisterTasklist(
   DWORD Unknown0)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserSetConsoleReserveKeys(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserSetDbgTag(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserSetRipFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserSetSysColors(
   int cElements,
   IN CONST INT *lpaElements,
   IN CONST COLORREF *lpaRgbValues,
   FLONG Flags)
{
   DWORD Ret = TRUE;

   if (cElements == 0)
      return TRUE;

   /* We need this check to prevent overflow later */
   if ((ULONG)cElements >= 0x40000000)
   {
      EngSetLastError(ERROR_NOACCESS);
      return FALSE;
   }

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead(lpaElements, cElements * sizeof(INT), 1);
      ProbeForRead(lpaRgbValues, cElements * sizeof(COLORREF), 1);

      IntSetSysColors(cElements, lpaElements, lpaRgbValues);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      Ret = FALSE;
   }
   _SEH2_END;

   if (Ret)
   {
      UserSendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

      UserRedrawDesktop();
   }

   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserUpdateInputContext(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserUpdateInstance(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserUserHandleGrantAccess(
   IN HANDLE hUserHandle,
   IN HANDLE hJob,
   IN BOOL bGrant)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserWaitForMsgAndEvent(
   DWORD Unknown0)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserWin32PoolAllocationStats(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserYieldTask(VOID)
{
   STUB

   return 0;
}

NTSTATUS
APIENTRY
NtUserConsoleControl(
    IN CONSOLECONTROL ConsoleCtrl,
    IN PVOID ConsoleCtrlInfo,
    IN ULONG ConsoleCtrlInfoLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Allow only Console Server to perform this operation (via CSRSS) */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    switch (ConsoleCtrl)
    {
        case GuiConsoleWndClassAtom:
        {
            if (ConsoleCtrlInfoLength != sizeof(ATOM))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                ProbeForRead(ConsoleCtrlInfo, ConsoleCtrlInfoLength, 1);
                gaGuiConsoleWndClass = *(ATOM*)ConsoleCtrlInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case ConsoleMakePalettePublic:
        {
            HPALETTE hPalette;

            if (ConsoleCtrlInfoLength != sizeof(HPALETTE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                ProbeForRead(ConsoleCtrlInfo, ConsoleCtrlInfoLength, 1);
                hPalette = *(HPALETTE*)ConsoleCtrlInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Make the palette handle public */
            GreSetObjectOwnerEx(hPalette,
                                GDI_OBJ_HMGR_PUBLIC,
                                GDIOBJFLAG_IGNOREPID);

            break;
        }

        case ConsoleAcquireDisplayOwnership:
        {
            ERR("NtUserConsoleControl - ConsoleAcquireDisplayOwnership is UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        default:
            ERR("Calling invalid control %lu in NtUserConsoleControl\n", ConsoleCtrl);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    UserLeave();

    return Status;
}

DWORD
APIENTRY
NtUserCreateInputContext(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserDestroyInputContext(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

BOOL
NTAPI
NtUserNotifyProcessCreate(
    HANDLE NewProcessId,
    HANDLE ParentThreadId,
    ULONG  dwUnknown,
    ULONG  CreateFlags)
{
    STUB;
    return FALSE;
}

NTSTATUS
APIENTRY
NtUserProcessConnect(
    IN  HANDLE ProcessHandle,
    OUT PUSERCONNECT pUserConnect,
    IN  ULONG Size)
{
    NTSTATUS Status;
    PEPROCESS Process = NULL;
    PPROCESSINFO W32Process;

    TRACE("NtUserProcessConnect\n");

    if ( pUserConnect == NULL ||
         Size         != sizeof(*pUserConnect) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the process object the user handle was referencing */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       *PsProcessType,
                                       UserMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    UserEnterShared();

    /* Get Win32 process information */
    W32Process = PsGetProcessWin32Process(Process);

    _SEH2_TRY
    {
        // FIXME: Check that pUserConnect->ulVersion == USER_VERSION;

        ProbeForWrite(pUserConnect, sizeof(*pUserConnect), sizeof(PVOID));
        pUserConnect->siClient.psi = gpsi;
        pUserConnect->siClient.aheList = gHandleTable;
        pUserConnect->siClient.ulSharedDelta =
            (ULONG_PTR)W32Process->HeapMappings.KernelMapping -
            (ULONG_PTR)W32Process->HeapMappings.UserMapping;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
        SetLastNtError(Status);

    UserLeave();

    /* Dereference the process object */
    ObDereferenceObject(Process);

    return Status;
}

NTSTATUS
APIENTRY
NtUserQueryInformationThread(IN HANDLE ThreadHandle,
                             IN USERTHREADINFOCLASS ThreadInformationClass,
                             OUT PVOID ThreadInformation,
                             IN ULONG ThreadInformationLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;

    /* Allow only CSRSS to perform this operation */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    /* Get the Thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_QUERY_INFORMATION,
                                       *PsThreadType,
                                       UserMode,
                                       (PVOID)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Quit;

    switch (ThreadInformationClass)
    {
        default:
        {
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    ObDereferenceObject(Thread);

Quit:
    UserLeave();
    return Status;
}

DWORD
APIENTRY
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserRealInternalGetMessage(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bGMSG)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserRealWaitMessageEx(
    DWORD dwWakeMask,
    UINT uTimeout)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

NTSTATUS
APIENTRY
NtUserSetInformationThread(IN HANDLE ThreadHandle,
                           IN USERTHREADINFOCLASS ThreadInformationClass,
                           IN PVOID ThreadInformation,
                           IN ULONG ThreadInformationLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;
    HANDLE CsrPortHandle;

    /* Allow only CSRSS to perform this operation */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    /* Get the Thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_INFORMATION,
                                       *PsThreadType,
                                       UserMode,
                                       (PVOID)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Quit;

    switch (ThreadInformationClass)
    {
        case UserThreadInitiateShutdown:
        {
            ERR("Shutdown initiated\n");
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        case UserThreadEndShutdown:
        {
            ERR("Shutdown ended\n");
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        case UserThreadCsrApiPort:
        {
            ERR("Set CSR API Port for Win32k\n");

            if (ThreadInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForRead(ThreadInformation, sizeof(HANDLE), sizeof(PVOID));
                CsrPortHandle = *(PHANDLE)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            if (NT_SUCCESS(Status))
            {
                Status = InitCsrApiPort(CsrPortHandle);
            }
            break;
        }

        default:
        {
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    ObDereferenceObject(Thread);

Quit:
    UserLeave();
    return Status;
}

DWORD
APIENTRY
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserSoundSentry(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserTestForInteractiveUser(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

/* http://www.cyber-ta.org/releases/malware-analysis/public/SOURCES/b47155634ccb2c30630da7e3666d3d07/b47155634ccb2c30630da7e3666d3d07.trace.html#NtUserGetIconSize */
DWORD
APIENTRY
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6)
{
    STUB;
    return 0;
}


DWORD
APIENTRY
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawScreen(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteStopScreenUpdates(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT hRect,
   HMENU hMenu,
   HFONT hFont)
{
   /* We'll use this function just for caching the menu bar */
   STUB
   return 0;
}

/*
 * FillWindow: Called from User; Dialog, Edit and ListBox procs during a WM_ERASEBKGND.
 */
/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFillWindow(HWND hWndPaint,
                 HWND hWndPaint1,
                 HDC  hDC,
                 HBRUSH hBrush)
{
   STUB

   return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserLockWindowUpdate(HWND hWnd)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  STUB;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserUpdateLayeredWindow(
   HWND hwnd,
   HDC hdcDst,
   POINT *pptDst,
   SIZE *psize,
   HDC hdcSrc,
   POINT *pptSrc,
   COLORREF crKey,
   BLENDFUNCTION *pblend,
   DWORD dwFlags,
   RECT *prcDirty)
{
   STUB

   return 0;
}

/*
 * NtUserResolveDesktopForWOW
 *
 * Status
 *    @unimplemented
 */

DWORD APIENTRY
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
   STUB
   return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserEndMenu(VOID)
{
   STUB

   return 0;
}

/*
 * @implemented
 */
/* NOTE: unused function */
BOOL APIENTRY
NtUserTrackPopupMenuEx(
   HMENU hMenu,
   UINT fuFlags,
   int x,
   int y,
   HWND hWnd,
   LPTPMPARAMS lptpm)
{
   STUB

   return FALSE;
}

DWORD APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
    STUB;

    return 0;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
   STUB

   return(0);
}

/*
 * NtUserSetWindowStationUser
 *
 * Status
 *    @unimplemented
 */

DWORD APIENTRY
NtUserSetWindowStationUser(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   STUB

   return 0;
}

BOOL APIENTRY NtUserAddClipboardFormatListener(
    HWND hwnd
)
{
    STUB;
    return FALSE;
}

BOOL APIENTRY NtUserRemoveClipboardFormatListener(
    HWND hwnd
)
{
    STUB;
    return FALSE;
}

BOOL APIENTRY NtUserGetUpdatedClipboardFormats(
    PUINT lpuiFormats,
    UINT cFormats,
    PUINT pcFormatsOut
)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserSetSystemCursor(
    HCURSOR hcur,
    DWORD id)
{
    return FALSE;
}

/* EOF */
