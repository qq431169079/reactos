/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              IDirectDraw7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

/*
 * IMPLEMENT
 * Status ok
 */

#include "rosdraw.h"

HRESULT 
WINAPI 
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface, 
								REFIID id, 
								LPVOID *obj) 
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        *obj = &This->lpVtbl;
    }
    else if (IsEqualGUID(&IID_IDirectDraw, id))
    {
        *obj = &This->lpVtbl_v1;
    }
    else if (IsEqualGUID(&IID_IDirectDraw2, id))
    {
        *obj = &This->lpVtbl_v2;
    }
    else if (IsEqualGUID(&IID_IDirectDraw4, id))
    {
        *obj = &This->lpVtbl_v4;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    Main_DirectDraw_AddRef(iface);
    return S_OK;
}
/*
 * IMPLEMENT
 * Status ok
 */
ULONG
WINAPI 
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	   
	if (iface!=NULL)
	{
		This->mDDrawLocal.dwLocalRefCnt++;
        This->Ref++;  

		if (This->mDDrawLocal.lpGbl != NULL)
		{
			This->mDDrawLocal.lpGbl->dwRefCnt++;
		}
	}    
    return This->Ref;
}

/*
 * IMPLEMENT
 * Status ok
 */
ULONG 
WINAPI 
Main_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	
	if (iface!=NULL)
	{	  	
		This->mDDrawLocal.dwLocalRefCnt--;
        This->Ref--;  

		if (This->mDDrawLocal.lpGbl != NULL)
		{
			This->mDDrawLocal.lpGbl->dwRefCnt--;
		}
            
		if ( This->Ref == 0)
		{
			// set resoltion back to the one in registry
			if(This->cooperative_level & DDSCL_EXCLUSIVE)
			{
				ChangeDisplaySettings(NULL, 0);
			}
            
			Cleanup(iface);					
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}
    }
    return This->Ref;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
	/* MSDN say not implement but my question what does it return then */
    DX_WINDBG_trace();
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, 
							  DWORD dwFlags, 
                              LPDIRECTDRAWCLIPPER *ppClipper, 
							  IUnknown *pUnkOuter)
{
    DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
        return CLASS_E_NOAGGREGATION; 

    IDirectDrawClipperImpl* That; 
    That = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawClipperImpl));

    if (That == NULL) 
        return E_OUTOFMEMORY;
   
    That->lpVtbl = &DirectDrawClipper_Vtable;
    That->ref = 1;
    *ppClipper = (LPDIRECTDRAWCLIPPER)That;

    return That->lpVtbl->Initialize (*ppClipper, (LPDIRECTDRAW)iface, dwFlags);
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	IDirectDrawPaletteImpl* That; 
	DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
	{
        return CLASS_E_NOAGGREGATION; 
	}
    
	if(!This->cooperative_level)
    {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

	if (This->mDdCreatePalette.CreatePalette == NULL)
	{
		return DDERR_NODRIVERSUPPORT;
	}

    That = (IDirectDrawPaletteImpl*)DxHeapMemAlloc(sizeof(IDirectDrawPaletteImpl));
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}

    That->lpVtbl = &DirectDrawPalette_Vtable;    
    *ppPalette = (LPDIRECTDRAWPALETTE)That;

	That->DDPalette.dwRefCnt = 1;
    That->DDPalette.lpDD_lcl = &This->mDDrawLocal;
	That->DDPalette.dwProcessId = GetCurrentProcessId();
    That->DDPalette.lpColorTable = palent;

    if (dwFlags & DDPCAPS_1BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_2 ;
	}
	if (dwFlags & DDPCAPS_2BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_4 ;
	}
	if (dwFlags & DDPCAPS_4BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_16 ;
	}
	if (dwFlags & DDPCAPS_8BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_256 ;
	}

	if (dwFlags & DDPCAPS_ALPHA)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_ALPHA;
	}
	if (dwFlags & DDPCAPS_ALLOW256)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_ALLOW256 ;
	}
	/* FIXME see 
	   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceddk5/html/wce50lrfddrawiddrawpalettegbl.asp
     
	if (dwFlags & DDPCAPS_8BITENTRIES)
	{
		That->DDPalette.dwFlags |= 0;
	}
		
	if (dwFlags & DDPCAPS_INITIALIZE)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_PRIMARYSURFACE)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_PRIMARYSURFACELEFT)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_VSYNC)
	{
		That->DDPalette.dwFlags |= 0;
	}
    */


	/*  We need fill in this right 
	   That->DDPalette.hHELGDIPalette/dwReserved1 
       That->DDPalette.dwContentsStamp 
       That->DDPalette.dwSaveStamp 
     */
	
	This->mDdCreatePalette.lpDDPalette = &That->DDPalette;
	This->mDdCreatePalette.lpColorTable = palent;
	
	if (This->mDdCreatePalette.CreatePalette(&This->mDdCreatePalette) == DDHAL_DRIVER_HANDLED);
    {				
		if (This->mDdSetMode.ddRVal == DD_OK)
		{
			Main_DirectDraw_AddRef(iface);
			return That->lpVtbl->Initialize (*ppPalette, (LPDIRECTDRAW)iface, dwFlags, palent);
		}
	}

	return  DDERR_NODRIVERSUPPORT;	 		    			    
}

HRESULT internal_CreateNewSurface(IDirectDrawImpl* This, IDirectDrawSurfaceImpl* That);
/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    DX_WINDBG_trace();

    DxSurf *surf;
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    IDirectDrawSurfaceImpl* That; 
	DEVMODE DevMode; 
	LONG extra_surfaces = 0;

    if (pUnkOuter!=NULL) 
	{
        return CLASS_E_NOAGGREGATION; 
	}

    if(sizeof(DDSURFACEDESC2)!=pDDSD->dwSize && sizeof(DDSURFACEDESC)!=pDDSD->dwSize)
	{
        return DDERR_UNSUPPORTED;
	}
    
    That = (IDirectDrawSurfaceImpl*)DxHeapMemAlloc(sizeof(IDirectDrawSurfaceImpl));
    
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}

    surf = (DxSurf*)DxHeapMemAlloc(sizeof(DxSurf));

    if (surf == NULL) 
	{
		DxHeapMemFree(That);
        return E_OUTOFMEMORY;
	}

    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpVtbl_v3 = &DDRAW_IDDS3_Thunk_VTable;
	*ppSurf = (LPDIRECTDRAWSURFACE7)That;

    // FIXME free This->mDDrawGlobal.dsList  on release 
	if (This->mDDrawGlobal.dsList == NULL)
	{
		This->mDDrawGlobal.dsList = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT));  
		if (This->mDDrawGlobal.dsList == NULL)
		{			
			DxHeapMemFree(surf);
			DxHeapMemFree(That);
            return E_OUTOFMEMORY;
		}

		That->Owner = (IDirectDrawImpl *)This;
		That->Owner->mDDrawGlobal.dsList->dwIntRefCnt =1;

		/* we alwasy set to use the DirectDrawSurface7_Vtable as internel */
		That->Owner->mDDrawGlobal.dsList->lpVtbl = (PVOID) &DirectDrawSurface7_Vtable;
	}
      
    That->Surf = surf;

	/* Code from wine cvs 24/7-2006 */

	if (!(pDDSD->dwFlags & DDSD_CAPS))
    {
        /* DVIDEO.DLL does forget the DDSD_CAPS flag ... *sigh* */
        pDDSD->dwFlags |= DDSD_CAPS;
    }
    if (pDDSD->ddsCaps.dwCaps == 0)
    {
        /* This has been checked on real Windows */
        pDDSD->ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }

    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
    {
        /* If the surface is of the 'alloconload' type, ignore the LPSURFACE field */
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if ((pDDSD->dwFlags & DDSD_LPSURFACE) && (pDDSD->lpSurface == NULL))
    {
        /* Frank Herbert's Dune specifies a null pointer for the surface, ignore the LPSURFACE field */       
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

	 
	/* continue with my own code */
	memcpy(&That->Surf->mddsdPrimary,pDDSD,pDDSD->dwSize);
    That->Surf->mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC2);     

	memset(&DevMode,0,sizeof(DEVMODE));
	DevMode.dmSize = (WORD)sizeof(DEVMODE);
	DevMode.dmDriverExtra = 0;

	EnumDisplaySettingsEx(NULL, ENUM_CURRENT_SETTINGS, &DevMode, 0);
	if(!(pDDSD->dwFlags & DDSD_PIXELFORMAT))
	{			
		That->Surf->mddsdPrimary.dwFlags |= DDSD_PIXELFORMAT;
        That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
		
		if(pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
			/* FIXME */
			DX_STUB_str("DDSCAPS_ZBUFFER not implement");
			switch(pDDSD->dwMipMapCount) 
            {
                case 15:            
                    break;
                case 16:                    
                    break;
                case 24:                   
                    break;
                case 32:                    
                    break;
                default:
                    DX_STUB_str("nknown Z buffer bit depth");
            }
		}
		else
		{
			switch(DevMode.dmBitsPerPel)
			{
				case  8:					 
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFlags = DDPF_RGB;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFourCC = 0;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRGBBitCount=8;
					/* FIXME right value */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwGBitMask = 0; /* Green bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwBBitMask = 0; /* Blue bitmask */
				break;

				case 15:
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFlags = DDPF_RGB;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFourCC = 0;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRGBBitCount=15;
					/* FIXME right value */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRBitMask = 0x7C00; /* red bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwGBitMask = 0x3E0; /* Green bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
				break;

				case 16: 
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFlags = DDPF_RGB;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFourCC = 0;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRGBBitCount=16;
					/* FIXME right value */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRBitMask = 0xF800; /* red bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwGBitMask = 0x7E0; /* Green bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
				break;

				case 24: 
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFlags = DDPF_RGB;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFourCC = 0;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRGBBitCount=24;
					/* FIXME right value */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
				break;

				case 32: 
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFlags = DDPF_RGB;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwFourCC = 0;
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRGBBitCount=8;
					/* FIXME right value */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
					That->Surf->mddsdPrimary.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
				break;

				default:
				break;          
			}
		}	/* end else */					
	} /* end if(!(pDDSD->dwFlags & DDSD_PIXELFORMAT)) */

	/* Code from wine cvs 24/7-2006 */
    if(!(pDDSD->dwFlags & DDSD_WIDTH) || !(pDDSD->dwFlags & DDSD_HEIGHT) )
    {
        HWND window = (HWND) This->mDDrawGlobal.lpExclusiveOwner->hWnd;

        /* Fallback: From WineD3D / original mode */
        That->Surf->mddsdPrimary.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;

        That->Surf->mddsdPrimary.dwWidth =DevMode.dmPelsWidth;
        That->Surf->mddsdPrimary.dwHeight = DevMode.dmPelsHeight;
      
        if( (window != 0) )
        {
            RECT rect;
            if(GetWindowRect(window, &rect) )
            {
                /* This is a hack until I find a better solution */
                if( (rect.right - rect.left) <= 1 ||
                    (rect.bottom - rect.top) <= 1 )
                {
					/*
                    FIXME("Wanted to get surface dimensions from window %p, but it has only \
                           a size of %ldx%ld. Using full screen dimensions\n",
                           window, rect.right - rect.left, rect.bottom - rect.top);
				   */
                }
                else
                {
                    /* Not sure if this is correct */
                    That->Surf->mddsdPrimary.dwWidth = rect.right - rect.left;
                    That->Surf->mddsdPrimary.dwHeight = rect.bottom - rect.top;
                    /* TRACE("Using window %p's dimensions: %ldx%ld\n", window, desc2.dwWidth, desc2.dwHeight); */
                }
            }
        }
	}

	/* Mipmap count fixes */
    if(That->Surf->mddsdPrimary.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        if(That->Surf->mddsdPrimary.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            if(!That->Surf->mddsdPrimary.dwFlags & DDSD_MIPMAPCOUNT)
            {            
                /* Undocumented feature: Create sublevels until
                 * either the width or the height is 1
                 */
                DWORD min = That->Surf->mddsdPrimary.dwWidth < That->Surf->mddsdPrimary.dwHeight ? 
					        That->Surf->mddsdPrimary.dwWidth : That->Surf->mddsdPrimary.dwHeight;

                That->Surf->mddsdPrimary.dwMipMapCount = 0;
                while( min )
                {
                    That->Surf->mddsdPrimary.dwMipMapCount += 1;
                    min >>= 1;
                }
            }
        }
        else
        {            
            That->Surf->mddsdPrimary.dwMipMapCount = 1;
        }
        extra_surfaces = That->Surf->mddsdPrimary.dwMipMapCount - 1;        
        That->Surf->mddsdPrimary.dwFlags |= DDSD_MIPMAPCOUNT;
    }

	if( (That->Surf->mddsdPrimary.dwFlags & DDSD_CAPS) && 
		(That->Surf->mddsdPrimary.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
    {
        That->Surf->mddsdPrimary.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
    }

    

  



    
	
  
          
    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {              
           
         return internal_CreateNewSurface( This,  That);

    }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {       
			 return internal_CreateNewSurface( This,  That);

            memset(&That->Surf->mddsdOverlay, 0, sizeof(DDSURFACEDESC));
            memcpy(&That->Surf->mddsdOverlay, pDDSD, sizeof(DDSURFACEDESC));
            That->Surf->mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
            That->Surf->mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;
            That->Surf->mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
            
            That->Surf->mddsdOverlay.dwWidth = pDDSD->dwWidth;  //pels;
            That->Surf->mddsdOverlay.dwHeight = pDDSD->dwHeight; // lines;
            That->Surf->mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

            That->Surf->mddsdOverlay.ddpfPixelFormat.dwSize = pDDSD->ddpfPixelFormat.dwSize;
            That->Surf->mddsdOverlay.ddpfPixelFormat.dwFlags = pDDSD->ddpfPixelFormat.dwFlags;
            That->Surf->mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = pDDSD->ddpfPixelFormat.dwRGBBitCount;
                                     
            DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
            mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
            mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
            mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
            mDdCanCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay; // pDDSD;

            if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
            {              
                return DDERR_NOTINITIALIZED;
            }

            if (mDdCanCreateSurface.ddRVal != DD_OK)
            {
                return DDERR_NOTINITIALIZED;
            }

 
           memset(&That->Surf->mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           That->Surf->mOverlayGlobal.dwGlobalFlags = 0;
           That->Surf->mOverlayGlobal.lpDD       = &This->mDDrawGlobal;
           That->Surf->mOverlayGlobal.lpDDHandle = &This->mDDrawGlobal;
           That->Surf->mOverlayGlobal.wWidth  = (WORD)That->Surf->mddsdOverlay.dwWidth;
           That->Surf->mOverlayGlobal.wHeight = (WORD)That->Surf->mddsdOverlay.dwHeight;
           That->Surf->mOverlayGlobal.lPitch  = -1;
           That->Surf->mOverlayGlobal.ddpfSurface = That->Surf->mddsdOverlay.ddpfPixelFormat;
                      
           memset(&That->Surf->mOverlayMore[0], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           That->Surf->mOverlayMore[0].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           memset(&That->Surf->mOverlayLocal[0],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           That->Surf->mOverlayLocal[0].lpGbl = &That->Surf->mOverlayGlobal;
           That->Surf->mOverlayLocal[0].lpSurfMore = &That->Surf->mOverlayMore[0];
           That->Surf-> mOverlayLocal[0].dwProcessId = GetCurrentProcessId();
           That->Surf->mOverlayLocal[0].dwFlags = DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER;

           That->Surf->mOverlayLocal[0].dwFlags |= 
                                DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM|
                                DDRAWISURF_HASPIXELFORMAT|
                                DDRAWISURF_HASOVERLAYDATA;

            That->Surf->mOverlayLocal[0].ddsCaps.dwCaps = That->Surf->mddsdOverlay.ddsCaps.dwCaps;
            That->Surf->mpOverlayLocals[0] = &That->Surf->mOverlayLocal[0];
            

           DDHAL_CREATESURFACEDATA      mDdCreateSurface;
           mDdCreateSurface.lpDD = &This->mDDrawGlobal;
           mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
           mDdCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay;//pDDSD;
           mDdCreateSurface.lplpSList = That->Surf->mpOverlayLocals; //cSurfaces;
           mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

           if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           {
	            return DDERR_NOTINITIALIZED;
            }
  

            if (mDdCreateSurface.ddRVal != DD_OK) 
            {   
                return mDdCreateSurface.ddRVal;
            }

/*
            DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;
            mDdUpdateOverlay.lpDD = &This->mDDrawGlobal;
            mDdUpdateOverlay.UpdateOverlay = This->mCallbacks.HALDDSurface.UpdateOverlay;
            mDdUpdateOverlay.lpDDDestSurface = This->mpPrimaryLocals[0];
            mDdUpdateOverlay.lpDDSrcSurface = That->Surf->mpOverlayLocals[0];//pDDSurface;
            mDdUpdateOverlay.dwFlags = DDOVER_SHOW;

  
            mDdUpdateOverlay.rDest.top = 0;
            mDdUpdateOverlay.rDest.left = 0;
            mDdUpdateOverlay.rDest.right = 50;
            mDdUpdateOverlay.rDest.bottom = 50;

            mDdUpdateOverlay.rSrc.top = 0;
            mDdUpdateOverlay.rSrc.left = 0;
            mDdUpdateOverlay.rSrc.right = 50;
            mDdUpdateOverlay.rSrc.bottom = 50;

            if ( mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
            {
	            return DDERR_NOTINITIALIZED;
            }
  
            if (mDdUpdateOverlay.ddRVal != DD_OK) 
            {   
                return mDdUpdateOverlay.ddRVal;
            }
*/
           
            That->Surf->mpInUseSurfaceLocals[0] = That->Surf->mpOverlayLocals[0];            
            return DD_OK;          
        }	
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
        {
           DX_STUB_str( "Can not create backbuffer surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        {
           DX_STUB_str( "Can not create texture surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
           DX_STUB_str( "Can not create zbuffer surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) 
        {
           DX_STUB_str( "Can not create offscreenplain surface");
        }
  
    return DDERR_INVALIDSURFACETYPE;  
   
}

HRESULT 
internal_CreateNewSurface(IDirectDrawImpl* This, IDirectDrawSurfaceImpl* That)
{
           This->mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; 
           This->mDdCanCreateSurface.lpDDSurfaceDesc = (DDSURFACEDESC*)&That->Surf->mddsdPrimary; 

           if (This->mDdCanCreateSurface.CanCreateSurface(&This->mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           {         
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCanCreateSurface.ddRVal != DD_OK)
           {
              return DDERR_NOTINITIALIZED;
           }

           memset(&That->Owner->mPrimaryGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           That->Owner->mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
           That->Owner->mPrimaryGlobal.lpDD       = &This->mDDrawGlobal;
           That->Owner->mPrimaryGlobal.lpDDHandle = &This->mDDrawGlobal;
           That->Owner->mPrimaryGlobal.wWidth  = (WORD)This->mpModeInfos[0].dwWidth;
           That->Owner->mPrimaryGlobal.wHeight = (WORD)This->mpModeInfos[0].dwHeight;
           That->Owner->mPrimaryGlobal.lPitch  = This->mpModeInfos[0].lPitch;

           memset(&That->Surf->mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           That->Surf->mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           memset(&That->Surf->mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           That->Surf->mPrimaryLocal.lpGbl = &That->Owner->mPrimaryGlobal;
           That->Surf->mPrimaryLocal.lpSurfMore = &That->Surf->mPrimaryMore;
           That->Surf->mPrimaryLocal.dwProcessId = GetCurrentProcessId();
	             
           That->Surf->mPrimaryLocal.ddsCaps.dwCaps = That->Surf->mddsdPrimary.ddsCaps.dwCaps;
           That->Surf->mpPrimaryLocals[0] = &That->Surf->mPrimaryLocal;
          
           This->mDdCreateSurface.lpDDSurfaceDesc = (DDSURFACEDESC*)&That->Surf->mddsdPrimary;
           This->mDdCreateSurface.lplpSList = That->Surf->mpPrimaryLocals;
           This->mDdCreateSurface.dwSCnt = This->mDDrawGlobal.dsList->dwIntRefCnt ; 

            
           if (This->mDdCreateSurface.CreateSurface(&This->mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           {
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCreateSurface.ddRVal != DD_OK) 
           {   
              return This->mDdCreateSurface.ddRVal;
           }
                              
           That->Surf->mpInUseSurfaceLocals[0] = &That->Surf->mPrimaryLocal;
           return DD_OK;
}

/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst) 
{
    DX_WINDBG_trace();
    DX_STUB;    
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{
     
	DX_STUB_DD_OK();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DDSURFACEDESC2 desc_callback;
    DEVMODE DevMode;   
    int iMode=0;
	
	RtlZeroMemory(&desc_callback, sizeof(DDSURFACEDESC2));
			    	   
    desc_callback.dwSize = sizeof(DDSURFACEDESC2);	

    desc_callback.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;

    if (dwFlags & DDEDM_REFRESHRATES)
    {
	    desc_callback.dwFlags |= DDSD_REFRESHRATE;
        desc_callback.dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
    }

  
     /* FIXME check if the mode are suppretd before sending it back  */

	memset(&DevMode,0,sizeof(DEVMODE));
	DevMode.dmSize = (WORD)sizeof(DEVMODE);
	DevMode.dmDriverExtra = 0;

    while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0))
    {
       
	   if (pDDSD)
	   {
	       if ((pDDSD->dwFlags & DDSD_WIDTH) && (pDDSD->dwWidth != DevMode.dmPelsWidth))
	       continue; 
	       if ((pDDSD->dwFlags & DDSD_HEIGHT) && (pDDSD->dwHeight != DevMode.dmPelsHeight))
		   continue; 
	       if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->ddpfPixelFormat.dwFlags & DDPF_RGB) &&
		   (pDDSD->ddpfPixelFormat.dwRGBBitCount != DevMode.dmBitsPerPel))
		    continue; 
       } 
	
       desc_callback.dwHeight = DevMode.dmPelsHeight;
	   desc_callback.dwWidth = DevMode.dmPelsWidth;
	   
       if (DevMode.dmFields & DM_DISPLAYFREQUENCY)
       {
            desc_callback.dwRefreshRate = DevMode.dmDisplayFrequency;
       }

	   if (desc_callback.dwRefreshRate == 0)
	   {
		   DX_STUB_str("dwRefreshRate = 0, we hard code it to value 60");
           desc_callback.dwRefreshRate = 60; /* Maybe the valye should be biger */
	   }

      /* above same as wine */
	  if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->ddpfPixelFormat.dwFlags & DDPF_RGB) )
	  {         
			switch(DevMode.dmBitsPerPel)
			{
				case  8:
					 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
					 desc_callback.ddpfPixelFormat.dwFourCC = 0;
					 desc_callback.ddpfPixelFormat.dwRGBBitCount=8;
					 /* FIXME right value */
					 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					 desc_callback.ddpfPixelFormat.dwGBitMask = 0; /* Green bitmask */
					 desc_callback.ddpfPixelFormat.dwBBitMask = 0; /* Blue bitmask */
					break;

				case 15:
					desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
					 desc_callback.ddpfPixelFormat.dwFourCC = 0;
					 desc_callback.ddpfPixelFormat.dwRGBBitCount=15;
					 /* FIXME right value */
					 desc_callback.ddpfPixelFormat.dwRBitMask = 0x7C00; /* red bitmask */
					 desc_callback.ddpfPixelFormat.dwGBitMask = 0x3E0; /* Green bitmask */
					 desc_callback.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
					break;

				case 16: 
					 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
					 desc_callback.ddpfPixelFormat.dwFourCC = 0;
					 desc_callback.ddpfPixelFormat.dwRGBBitCount=16;
					 /* FIXME right value */
					 desc_callback.ddpfPixelFormat.dwRBitMask = 0xF800; /* red bitmask */
					 desc_callback.ddpfPixelFormat.dwGBitMask = 0x7E0; /* Green bitmask */
					 desc_callback.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
					break;

				case 24: 
					 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
					 desc_callback.ddpfPixelFormat.dwFourCC = 0;
					 desc_callback.ddpfPixelFormat.dwRGBBitCount=24;
					 /* FIXME right value */
					 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					 desc_callback.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
					 desc_callback.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
					break;

				case 32: 
					 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
					 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
					 desc_callback.ddpfPixelFormat.dwFourCC = 0;
					 desc_callback.ddpfPixelFormat.dwRGBBitCount=8;
					 /* FIXME right value */
					 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
					 desc_callback.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
					 desc_callback.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
					break;

				default:
					break;          
			}
			desc_callback.ddsCaps.dwCaps = 0;
		    if (desc_callback.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) 
		    {
				/* FIXME srt DDCS Caps flag */
				desc_callback.ddsCaps.dwCaps |= DDSCAPS_PALETTE;
		    }    
	  }
				  
	  if (DevMode.dmBitsPerPel==15)
      {           
		  desc_callback.lPitch =  DevMode.dmPelsWidth + (8 - ( DevMode.dmPelsWidth % 8)) % 8;
	  }
	  else
	  {
		  desc_callback.lPitch = DevMode.dmPelsWidth * (DevMode.dmBitsPerPel  / 8);
	      desc_callback.lPitch = desc_callback.lPitch + (8 - (desc_callback.lPitch % 8)) % 8;
	  }
	  	 	  	                                                  
      if (callback(&desc_callback, context) == DDENUMRET_CANCEL)
      {
         return DD_OK;       
      }
       
      iMode++; 
    }

    return DD_OK;
}

/*
 * stub
 * Status not done
 */
HRESULT WINAPI 
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK7 callback) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	if (This->mDdFlipToGDISurface.FlipToGDISurface == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	This->mDdFlipToGDISurface.ddRVal = DDERR_NOTPALETTIZED;
	This->mDdFlipToGDISurface.dwToGDI = TRUE;

	if (This->mDdFlipToGDISurface.FlipToGDISurface(&This->mDdFlipToGDISurface)==DDHAL_DRIVER_HANDLED);
    {				
		return This->mDdFlipToGDISurface.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 		    	   
}
 
/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
            LPDDCAPS pHELCaps) 
{	
    DX_WINDBG_trace();

	DDSCAPS2 ddscaps;
    DWORD status = DD_FALSE;	
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
	
    if (pDriverCaps != NULL) 
    {                          
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->mDDrawGlobal.ddCaps.dwVidMemTotal, 
		                                 &This->mDDrawGlobal.ddCaps.dwVidMemFree);	 
	  
	  RtlCopyMemory(pDriverCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
	  pDriverCaps->dwSize=sizeof(DDCAPS);
	  
      status = DD_OK;
    }

    if (pHELCaps != NULL) 
    {      	  
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->mDDrawGlobal.ddHELCaps.dwVidMemTotal, 
		                                 &This->mDDrawGlobal.ddHELCaps.dwVidMemFree);	  	 

	  RtlCopyMemory(pDriverCaps,&This->mDDrawGlobal.ddHELCaps,sizeof(DDCORECAPS));
      status = DD_OK;
    }
    
    return status;
}


/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{   
    DX_WINDBG_trace();

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    if (pDDSD == NULL)
    {
      return DD_FALSE;
    }
        
    pDDSD->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE | DDSD_WIDTH; 
    pDDSD->dwHeight  = This->mDDrawGlobal.vmiData.dwDisplayHeight;
    pDDSD->dwWidth = This->mDDrawGlobal.vmiData.dwDisplayWidth; 
    pDDSD->lPitch  = This->mDDrawGlobal.vmiData.lDisplayPitch;
    pDDSD->dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
    pDDSD->dwAlphaBitDepth = This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth;

    RtlCopyMemory(&pDDSD->ddpfPixelFormat,&This->mDDrawGlobal.vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
    RtlCopyMemory(&pDDSD->ddsCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
    
    RtlCopyMemory(&pDDSD->ddckCKDestOverlay,&This->mDDrawGlobal.ddckCKDestOverlay,sizeof(DDCOLORKEY));
    RtlCopyMemory(&pDDSD->ddckCKSrcOverlay,&This->mDDrawGlobal.ddckCKSrcOverlay,sizeof(DDCOLORKEY));

    /* have not check where I should get hold of this info yet
	DWORD  dwBackBufferCount;    
    DWORD  dwReserved;
    LPVOID lpSurface;    
    DDCOLORKEY    ddckCKDestBlt;    
    DDCOLORKEY    ddckCKSrcBlt;  
    DWORD         dwTextureStage;
    */
  
    return DD_OK;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{  
    DX_WINDBG_trace();

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    if (freq == NULL)
    {
        return DD_FALSE;
    }

    *freq = This->mDDrawGlobal.dwMonitorFrequency;
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{    

	DX_WINDBG_trace();
	
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    *lpdwScanLine = 0;

	if (This->mDdGetScanLine.GetScanLine == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	This->mDdGetScanLine.ddRVal = DDERR_NOTPALETTIZED;
	This->mDdGetScanLine.dwScanLine = 0;

	if (This->mDdGetScanLine.GetScanLine(&This->mDdGetScanLine)==DDHAL_DRIVER_HANDLED);
    {			
		*lpdwScanLine = This->mDdGetScanLine.dwScanLine;
		return This->mDdGetScanLine.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 		    	       
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{       
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
       
	if (iface==NULL) 
	{
		return DDERR_NOTINITIALIZED;
	}

	if (This->InitializeDraw == TRUE)
	{
        return DDERR_ALREADYINITIALIZED;
	}
	else
	{
     This->InitializeDraw = TRUE;
    }

    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
   DX_WINDBG_trace();

   ChangeDisplaySettings(NULL, 0);
   return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
    // TODO:                                                            
    // - create a scaner that check which driver we should get the HDC from    
    //   for now we always asume it is the active dirver that should be use.
    // - allow more Flags

    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DDHAL_SETEXCLUSIVEMODEDATA SetExclusiveMode;
    
    // check the parameters
    if ((This->cooperative_level == cooplevel) && ((HWND)This->mDDrawGlobal.lpExclusiveOwner->hWnd  == hwnd))
        return DD_OK;
    
    if (This->cooperative_level)
        return DDERR_EXCLUSIVEMODEALREADYSET;

    if ((cooplevel&DDSCL_EXCLUSIVE) && !(cooplevel&DDSCL_FULLSCREEN))
        return DDERR_INVALIDPARAMS;

    if (cooplevel&DDSCL_NORMAL && cooplevel&DDSCL_FULLSCREEN)
        return DDERR_INVALIDPARAMS;

    // set the data
    This->mDDrawGlobal.lpExclusiveOwner->hWnd = (ULONG_PTR) hwnd;
    This->mDDrawGlobal.lpExclusiveOwner->hDC  = (ULONG_PTR)GetDC(hwnd);

	
	/* FIXME : fill the  mDDrawGlobal.lpExclusiveOwner->dwLocalFlags right */
	//mDDrawGlobal.lpExclusiveOwner->dwLocalFlags

    This->cooperative_level = cooplevel;

    if ((This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE)) 
    {       
        DX_STUB_str("HAL \n");
        SetExclusiveMode.SetExclusiveMode = This->mDDrawGlobal.lpDDCBtmp->HALDD.SetExclusiveMode;                            
    }
    else
    {
        DX_STUB_str("HEL \n");
        SetExclusiveMode.SetExclusiveMode = This->mDDrawGlobal.lpDDCBtmp->HELDD.SetExclusiveMode;
    }
             
    SetExclusiveMode.lpDD = &This->mDDrawGlobal;
    SetExclusiveMode.ddRVal = DDERR_NOTPALETTIZED;
    SetExclusiveMode.dwEnterExcl = This->cooperative_level;
     
    if (SetExclusiveMode.SetExclusiveMode(&SetExclusiveMode) != DDHAL_DRIVER_HANDLED)
    {
        return DDERR_NODRIVERSUPPORT;
    }

    return SetExclusiveMode.ddRVal;               
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	BOOL dummy = TRUE;
	DEVMODE DevMode; 	
    int iMode=0;
    int Width=0;
    int Height=0;
    int BPP=0;
	
	/* FIXME check the refresrate if it same if it not same do the mode switch */
	if ((This->mDDrawGlobal.vmiData.dwDisplayHeight == dwHeight) && 
		(This->mDDrawGlobal.vmiData.dwDisplayWidth == dwWidth)  && 
		(This->mDDrawGlobal.vmiData.ddpfDisplay.dwRGBBitCount == dwBPP))  
		{
          
		  return DD_OK;
		}

	if (This->mDdSetMode.SetMode == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	/* Check use the Hal or Hel for SetMode */
	// this only for exclusive mode
	if(!(This->cooperative_level & DDSCL_EXCLUSIVE))
	{
   		return DDERR_NOEXCLUSIVEMODE;
	}

	DevMode.dmSize = (WORD)sizeof(DEVMODE);
	DevMode.dmDriverExtra = 0;

    while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0 ) != 0)
    {
	 
       if ((dwWidth == DevMode.dmPelsWidth) && (dwHeight == DevMode.dmPelsHeight) && ( dwBPP == DevMode.dmBitsPerPel))
	   {
		  Width = DevMode.dmPelsWidth;
		  Height = DevMode.dmPelsHeight;
          BPP = DevMode.dmBitsPerPel;		  
          break;   
	   }		   
	   iMode++;		   
    }

	if ((dwWidth != DevMode.dmPelsWidth) || (dwHeight != DevMode.dmPelsHeight) || ( dwBPP != DevMode.dmBitsPerPel))	
	{	
		return DDERR_UNSUPPORTEDMODE;
	}
	
	This->mDdSetMode.ddRVal = DDERR_NOTPALETTIZED;
	
    This->mDdSetMode.dwModeIndex = iMode; 
	This->mDdSetMode.SetMode(&This->mDdSetMode);
	
	DdReenableDirectDrawObject(&This->mDDrawGlobal, &dummy);

	/* FIXME fill the This->DirectDrawGlobal.vmiData right */		     
     //This->mDDrawGlobal.lpExclusiveOwner->hDC  = (ULONG_PTR)GetDC( (HWND)This->mDDrawGlobal.lpExclusiveOwner->hWnd);  
	return This->mDdSetMode.ddRVal;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                                   HANDLE h)
{
    DX_WINDBG_trace();
	
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    
	if (This->mDdWaitForVerticalBlank.WaitForVerticalBlank == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	This->mDdWaitForVerticalBlank.dwFlags = dwFlags;
    This->mDdWaitForVerticalBlank.hEvent = (DWORD)h;
    This->mDdWaitForVerticalBlank.ddRVal = DDERR_NOTPALETTIZED;

	if (This->mDdWaitForVerticalBlank.WaitForVerticalBlank(&This->mDdWaitForVerticalBlank)==DDHAL_DRIVER_HANDLED);
    {					
		return This->mDdWaitForVerticalBlank.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD total, LPDWORD free)                                               
{    
    DX_WINDBG_trace();
	DDHAL_GETAVAILDRIVERMEMORYDATA  mem;

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    
	/* Only Hal version exists acodring msdn */
	if (!(This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY))
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}
	
	mem.lpDD = This->mDDrawLocal.lpGbl;
    mem.ddRVal = DDERR_NOTPALETTIZED;	
    mem.DDSCaps.dwCaps = ddscaps->dwCaps;
    mem.ddsCapsEx.dwCaps2 = ddscaps->dwCaps2;
    mem.ddsCapsEx.dwCaps3 = ddscaps->dwCaps3;
    mem.ddsCapsEx.dwCaps4 = ddscaps->dwCaps4;

	if (This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous.GetAvailDriverMemory(&mem) == DDHAL_DRIVER_HANDLED);
    {
		if (total !=NULL)
		{
           *total = mem.dwTotal;
		}

        *free = mem.dwFree;
		return mem.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{  
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
                   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{  
    DX_WINDBG_trace();
    DX_STUB;
}


IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
