/* Copyright 2001 Mike McCormack
 * Copyright 2003 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "wine/debug.h"
#include "netbios.h"

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

HMODULE NETAPI32_hModule = 0;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL);
            NETAPI32_hModule = hinstDLL;
            NetBIOSInit();
            NetBTInit();
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            NetBIOSShutdown();
            break;
        }
    }

    return TRUE;
}

NET_API_STATUS  WINAPI NetServerEnum(
  LPCWSTR servername,
  DWORD level,
  LPBYTE* bufptr,
  DWORD prefmaxlen,
  LPDWORD entriesread,
  LPDWORD totalentries,
  DWORD servertype,
  LPCWSTR domain,
  LPDWORD resume_handle
)
{
    FIXME("Stub (%p, %ld %p %ld %p %p %ld %s %p)\n",servername, level, bufptr,
          prefmaxlen, entriesread, totalentries, servertype, debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

NET_API_STATUS WINAPI 
NetServerGetInfo(LPWSTR servername, DWORD level, LPBYTE* bufptr)
{
    FIXME("stub (%p, %ld, %p)\n", servername, level, bufptr);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 *                NetStatisticsGet  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetStatisticsGet(LPWSTR server, LPWSTR service,
                                       DWORD level, DWORD options,
                                       PBYTE *bufptr)
{
    TRACE("(%p, %p, %ld, %ld, %p)\n", server, service, level, options, bufptr);
    return NERR_InternalError;
}
