/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: fillshap.c,v 1.27 2003/08/20 07:45:02 gvg Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <win32k/region.h>
#include <include/error.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/path.h>
#include <include/paint.h>
#include <internal/safe.h>

//#define NDEBUG
#include <win32k/debug1.h>

BOOL
STDCALL
NtGdiChord(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiEllipse(HDC  hDC,
                  int  LeftRect,
                  int  TopRect,
                  int  RightRect,
                  int  BottomRect)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiPie(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XRadial1,
              int  YRadial1,
              int  XRadial2,
              int  YRadial2)
{
  UNIMPLEMENTED;
}

#if 0

//When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and 
//even-numbered polygon sides on each scan line. That is, GDI fills the area between the 
//first and second side, between the third and fourth side, and so on. 

//WINDING Selects winding mode (fills any region with a nonzero winding value). 
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value.
//This value is defined as the number of times a pen used to draw the polygon would go around the region.
//The direction of each edge of the polygon is important. 

extern BOOL FillPolygon(PDC dc,
				  SURFOBJ *SurfObj,
				  PBRUSHOBJ BrushObj,
				  MIX RopMode,
				  CONST PPOINT Points,
				  int Count,
				  RECTL BoundRect);

#endif

BOOL
FASTCALL
IntPolygon(PDC          dc,
           CONST PPOINT UnsafePoints,
           int          Count)
{
  SURFOBJ *SurfObj;
  BRUSHOBJ PenBrushObj, *FillBrushObj;
  BOOL ret = FALSE; // default to failure
  PRECTL RectBounds;
  RECTL DestRect;
  int CurrentPoint;
  PPOINT Points;
  NTSTATUS Status;

  ASSERT(dc); // caller's responsibility to pass a valid dc

  if ( NULL == UnsafePoints || Count < 2 )
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  ASSERT(SurfObj);

  /* Copy points from userspace to kernelspace */
  Points = ExAllocatePool(PagedPool, Count * sizeof(POINT));
  if (NULL == Points)
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
  else
  {
    Status = MmCopyFromCaller(Points, UnsafePoints, Count * sizeof(POINT));
    if ( !NT_SUCCESS(Status) )
      SetLastNtError(Status);
    else
    {
      /* Convert to screen coordinates */
      for (CurrentPoint = 0; CurrentPoint < Count; CurrentPoint++)
	{
	  Points[CurrentPoint].x += dc->w.DCOrgX;
	  Points[CurrentPoint].y += dc->w.DCOrgY;
	}

      RectBounds = (PRECTL) RGNDATA_LockRgn(dc->w.hGCClipRgn);
      //ei not yet implemented ASSERT(RectBounds);

      if (PATH_IsPathOpen(dc->w.path)) 
	ret = PATH_Polygon(dc, Points, Count );
      else
      {
	DestRect.left   = Points[0].x;
	DestRect.right  = Points[0].x;
	DestRect.top    = Points[0].y;
	DestRect.bottom = Points[0].y;

	for (CurrentPoint = 1; CurrentPoint < Count; ++CurrentPoint)
	{
	  DestRect.left     = MIN(DestRect.left, Points[CurrentPoint].x);
	  DestRect.right    = MAX(DestRect.right, Points[CurrentPoint].x);
	  DestRect.top      = MIN(DestRect.top, Points[CurrentPoint].y);
	  DestRect.bottom   = MAX(DestRect.bottom, Points[CurrentPoint].y);
	}

#if 1
	/* Now fill the polygon with the current brush. */
	FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
	ASSERT(FillBrushObj);
	if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
	  ret = FillPolygon ( dc, SurfObj, FillBrushObj, dc->w.ROPmode, Points, Count, DestRect );
	BRUSHOBJ_UnlockBrush(dc->w.hBrush);
#endif

	/* make BRUSHOBJ from current pen. */
	HPenToBrushObj ( &PenBrushObj, dc->w.hPen );

	// Draw the Polygon Edges with the current pen ( if not a NULL pen )
	if ( PenBrushObj.logbrush.lbStyle != BS_NULL )
	{
	  for ( CurrentPoint = 0; CurrentPoint < Count; ++CurrentPoint )
	  {
	    POINT To, From; //, Next;

	    /* Let CurrentPoint be i
	     * if i+1 > Count, Draw a line from Points[i] to Points[0]
	     * Draw a line from Points[i] to Points[i+1]
	     */
	    From = Points[CurrentPoint];
	    if (Count <= CurrentPoint + 1)
	      To = Points[0];
	    else
	      To = Points[CurrentPoint + 1];

	    //DPRINT("Polygon Making line from (%d,%d) to (%d,%d)\n", From.x, From.y, To.x, To.y );
	    ret = IntEngLineTo(SurfObj,
			       dc->CombinedClip,
			       &PenBrushObj,
			       From.x,
			       From.y,
			       To.x,
			       To.y,
			       &DestRect,
			       dc->w.ROPmode); /* MIX */
	  }
	}
#if 0
	/* Now fill the polygon with the current brush. */
	FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
	ASSERT(FillBrushObj);
	if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
	  ret = FillPolygon ( dc, SurfObj, FillBrushObj, dc->w.ROPmode, Points, Count, DestRect );
	BRUSHOBJ_UnlockBrush(dc->w.hBrush);
#endif
      }

      RGNDATA_UnlockRgn(dc->w.hGCClipRgn);
    }
    ExFreePool ( Points );
  }
  return ret;
}

//This implementation is blatantly ripped off from NtGdiRectangle
BOOL
STDCALL
NtGdiPolygon(HDC          hDC,
            CONST PPOINT UnsafePoints,
            int          Count)
{
  DC *dc;
  BOOL ret = FALSE; // default to failure

  //DPRINT("In NtGdiPolygon()\n");

  dc = DC_LockDc ( hDC );

  if ( !dc )
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
  else
  {
    ret = IntPolygon ( dc, UnsafePoints, Count );
    DC_UnlockDc ( hDC );
  }

  return ret;
}


BOOL
STDCALL
NtGdiPolyPolygon(HDC            hDC,
                CONST LPPOINT  Points,
                CONST LPINT    PolyCounts,
                int            Count)
{
  UNIMPLEMENTED;
}

BOOL
FASTCALL
IntRectangle(PDC dc,
	     int LeftRect,
	     int TopRect,
	     int RightRect,
	     int BottomRect)
{
  SURFOBJ   *SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  BRUSHOBJ   PenBrushObj, *FillBrushObj;
  BOOL       ret = FALSE; // default to failure
  PRECTL     RectBounds;
  RECTL      DestRect;

  ASSERT ( dc ); // caller's responsibility to set this up

  RectBounds = (PRECTL) RGNDATA_LockRgn(dc->w.hGCClipRgn);
  //ei not yet implemented ASSERT(RectBounds);

  if ( PATH_IsPathOpen(dc->w.path) )
  {
    ret = PATH_Rectangle ( dc, LeftRect, TopRect, RightRect, BottomRect );
  }
  else
  {
    LeftRect   += dc->w.DCOrgX;
    RightRect  += dc->w.DCOrgX - 1;
    TopRect    += dc->w.DCOrgY;
    BottomRect += dc->w.DCOrgY - 1;

    FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);

    ASSERT(FillBrushObj); // FIXME - I *think* this should always happen...
    // it would be nice to remove the following if statement if that proves to be true
    if ( FillBrushObj )
    {
      if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
      {
	DestRect.left = LeftRect;
	DestRect.right = RightRect;
	DestRect.top = TopRect;
	DestRect.bottom = BottomRect;
	ret = ret && IntEngBitBlt(SurfObj,
				  NULL,
				  NULL,
				  NULL,
				  NULL,
				  &DestRect,
				  NULL,
				  NULL,
				  FillBrushObj,
				  NULL,
				  PATCOPY);
      }
    }

    BRUSHOBJ_UnlockBrush(dc->w.hBrush);

    /* make BRUSHOBJ from current pen. */
    HPenToBrushObj ( &PenBrushObj, dc->w.hPen );

    // Draw the rectangle with the current pen

    ret = TRUE; // change default to success

    if ( PenBrushObj.logbrush.lbStyle != BS_NULL )
    {
      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 LeftRect, TopRect, RightRect, TopRect,
			 RectBounds, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 RightRect, TopRect, RightRect, BottomRect,
			 RectBounds, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 RightRect, BottomRect, LeftRect, BottomRect,
			 RectBounds, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 LeftRect, BottomRect, LeftRect, TopRect,
			 RectBounds, // Bounding rectangle
			 dc->w.ROPmode); // MIX */
    }
  }

  // FIXME: Move current position in DC?
  RGNDATA_UnlockRgn(dc->w.hGCClipRgn);
  return TRUE;
}

BOOL
STDCALL
NtGdiRectangle(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect)
{
  DC   *dc = DC_LockDc(hDC);
  BOOL  ret = FALSE; // default to failure

  if ( dc )
  {
    ret = IntRectangle ( dc, LeftRect, TopRect, RightRect, BottomRect );
    DC_UnlockDc ( hDC );
  }

  return ret;
}

BOOL
STDCALL
NtGdiRoundRect(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect,
                    int  Width,
                    int  Height)
{
  UNIMPLEMENTED;
}
/* EOF */
