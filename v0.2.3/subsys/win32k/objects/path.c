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
/* $Id: path.c,v 1.21 2004/05/10 17:07:20 weiden Exp $ */
#include <w32k.h>
#include <win32k/float.h>

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */
#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */

BOOL FASTCALL PATH_AddEntry (GdiPath *pPath, const POINT *pPoint, BYTE flags);
BOOL FASTCALL PATH_AddFlatBezier (GdiPath *pPath, POINT *pt, BOOL closed);
BOOL FASTCALL PATH_DoArcPart (GdiPath *pPath, FLOAT_POINT corners[], double angleStart, double angleEnd, BOOL addMoveTo);
BOOL FASTCALL PATH_FlattenPath (GdiPath *pPath);
VOID FASTCALL PATH_GetPathFromDC (PDC dc, GdiPath **ppPath);
VOID FASTCALL PATH_NormalizePoint (FLOAT_POINT corners[], const FLOAT_POINT *pPoint, double *pX, double *pY);
BOOL FASTCALL PATH_PathToRegion(const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);
BOOL FASTCALL PATH_ReserveEntries (GdiPath *pPath, INT numEntries);
VOID FASTCALL PATH_ScaleNormalizedPoint (FLOAT_POINT corners[], double x, double y, POINT *pPoint);


INT FASTCALL
IntGdiGetArcDirection(DC *dc);

BOOL
STDCALL
NtGdiAbortPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiBeginPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
FASTCALL
IntCloseFigure ( PDC dc )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiCloseFigure ( HDC hDC )
{
  PDC dc = DC_LockDc ( hDC );
  BOOL ret = FALSE; // default to failure

  if ( dc )
  {
    ret = IntCloseFigure ( dc );
    DC_UnlockDc ( hDC );
  }

  return ret;
}

BOOL
STDCALL
NtGdiEndPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiFlattenPath(HDC  hDC)
{
  UNIMPLEMENTED;
}


BOOL
STDCALL
NtGdiGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
NtGdiPathToRegion(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiStrokeAndFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiStrokePath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiWidenPath(HDC  hDC)
{
   UNIMPLEMENTED;
}

/***********************************************************************
 * Exported functions
 */

/* PATH_InitGdiPath
 *
 * Initializes the GdiPath structure.
 */
VOID
FASTCALL
PATH_InitGdiPath ( GdiPath *pPath )
{
  assert(pPath!=NULL);

  pPath->state=PATH_Null;
  pPath->pPoints=NULL;
  pPath->pFlags=NULL;
  pPath->numEntriesUsed=0;
  pPath->numEntriesAllocated=0;
}

/* PATH_DestroyGdiPath
 *
 * Destroys a GdiPath structure (frees the memory in the arrays).
 */
VOID
FASTCALL
PATH_DestroyGdiPath ( GdiPath *pPath )
{
  assert(pPath!=NULL);

  ExFreePool(pPath->pPoints);
  ExFreePool(pPath->pFlags);
}

/* PATH_AssignGdiPath
 *
 * Copies the GdiPath structure "pPathSrc" to "pPathDest". A deep copy is
 * performed, i.e. the contents of the pPoints and pFlags arrays are copied,
 * not just the pointers. Since this means that the arrays in pPathDest may
 * need to be resized, pPathDest should have been initialized using
 * PATH_InitGdiPath (in C++, this function would be an assignment operator,
 * not a copy constructor).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_AssignGdiPath ( GdiPath *pPathDest, const GdiPath *pPathSrc )
{
  assert(pPathDest!=NULL && pPathSrc!=NULL);

  /* Make sure destination arrays are big enough */
  if ( !PATH_ReserveEntries(pPathDest, pPathSrc->numEntriesUsed) )
    return FALSE;

  /* Perform the copy operation */
  memcpy(pPathDest->pPoints, pPathSrc->pPoints,
    sizeof(POINT)*pPathSrc->numEntriesUsed);
  memcpy(pPathDest->pFlags, pPathSrc->pFlags,
    sizeof(BYTE)*pPathSrc->numEntriesUsed);

  pPathDest->state=pPathSrc->state;
  pPathDest->numEntriesUsed=pPathSrc->numEntriesUsed;
  pPathDest->newStroke=pPathSrc->newStroke;

  return TRUE;
}

/* PATH_MoveTo
 *
 * Should be called when a MoveTo is performed on a DC that has an
 * open path. This starts a new stroke. Returns TRUE if successful, else
 * FALSE.
 */
BOOL
FASTCALL
PATH_MoveTo ( PDC dc )
{
  GdiPath *pPath;

  /* Get pointer to path */
  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    /* FIXME: Do we have to call SetLastError? */
    return FALSE;

  /* Start a new stroke */
  pPath->newStroke = TRUE;

  return TRUE;
}

/* PATH_LineTo
 *
 * Should be called when a LineTo is performed on a DC that has an
 * open path. This adds a PT_LINETO entry to the path (and possibly
 * a PT_MOVETO entry, if this is the first LineTo in a stroke).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_LineTo ( PDC dc, INT x, INT y )
{
  GdiPath *pPath;
  POINT point, pointCurPos;

  /* Get pointer to path */
  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Convert point to device coordinates */
  point.x=x;
  point.y=y;
  CoordLPtoDP ( dc, &point );

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pointCurPos );
    CoordLPtoDP ( dc, &pointCurPos );
    if ( !PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO) )
      return FALSE;
  }

  /* Add a PT_LINETO entry */
  return PATH_AddEntry(pPath, &point, PT_LINETO);
}

/* PATH_Rectangle
 *
 * Should be called when a call to Rectangle is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Rectangle ( PDC dc, INT x1, INT y1, INT x2, INT y2 )
{
  GdiPath *pPath;
  POINT corners[2], pointTemp;
  INT   temp;

  /* Get pointer to path */
  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Convert points to device coordinates */
  corners[0].x=x1;
  corners[0].y=y1;
  corners[1].x=x2;
  corners[1].y=y2;
  IntLPtoDP ( dc, corners, 2 );

  /* Make sure first corner is top left and second corner is bottom right */
  if ( corners[0].x > corners[1].x )
  {
    temp=corners[0].x;
    corners[0].x=corners[1].x;
    corners[1].x=temp;
  }
  if ( corners[0].y > corners[1].y )
  {
    temp=corners[0].y;
    corners[0].y=corners[1].y;
    corners[1].y=temp;
  }

  /* In GM_COMPATIBLE, don't include bottom and right edges */
  if ( IntGetGraphicsMode(dc) == GM_COMPATIBLE )
  {
    corners[1].x--;
    corners[1].y--;
  }

  /* Close any previous figure */
  if ( !IntCloseFigure ( dc ) )
  {
    /* The NtGdiCloseFigure call shouldn't have failed */
    assert(FALSE);
    return FALSE;
  }

  /* Add four points to the path */
  pointTemp.x=corners[1].x;
  pointTemp.y=corners[0].y;
  if ( !PATH_AddEntry(pPath, &pointTemp, PT_MOVETO) )
    return FALSE;
  if ( !PATH_AddEntry(pPath, corners, PT_LINETO) )
    return FALSE;
  pointTemp.x=corners[0].x;
  pointTemp.y=corners[1].y;
  if ( !PATH_AddEntry(pPath, &pointTemp, PT_LINETO) )
    return FALSE;
  if ( !PATH_AddEntry(pPath, corners+1, PT_LINETO) )
    return FALSE;

  /* Close the rectangle figure */
  if ( !IntCloseFigure ( dc ) )
  {
    /* The IntCloseFigure call shouldn't have failed */
    assert(FALSE);
    return FALSE;
  }

  return TRUE;
}

BOOL
FASTCALL
PATH_RoundRect (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xradius, INT yradius)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* PATH_Ellipse
 *
 * Should be called when a call to Ellipse is performed on a DC that has
 * an open path. This adds four Bezier splines representing the ellipse
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Ellipse ( PDC dc, INT x1, INT y1, INT x2, INT y2 )
{
  /* TODO: This should probably be revised to call PATH_AngleArc */
  /* (once it exists) */
  return PATH_Arc ( dc, x1, y1, x2, y2, x1, (y1+y2)/2, x1, (y1+y2)/2 );
}

/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Arc ( PDC dc, INT x1, INT y1, INT x2, INT y2,
   INT xStart, INT yStart, INT xEnd, INT yEnd)
{
  GdiPath *pPath;
  DC      *pDC;
  double  angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant=0.0;
          /* Initialize angleEndQuadrant to silence gcc's warning */
  double  x, y;
  FLOAT_POINT corners[2], pointStart, pointEnd;
  BOOL    start, end;
  INT     temp;
  BOOL    clockwise;

  /* FIXME: This function should check for all possible error returns */
  /* FIXME: Do we have to respect newStroke? */

  ASSERT ( dc );

  clockwise = ( IntGdiGetArcDirection(dc) == AD_CLOCKWISE );

  /* Get pointer to path */
  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* FIXME: Do we have to close the current figure? */

  /* Check for zero height / width */
  /* FIXME: Only in GM_COMPATIBLE? */
  if ( x1==x2 || y1==y2 )
    return TRUE;

  /* Convert points to device coordinates */
  corners[0].x=(FLOAT)x1;
  corners[0].y=(FLOAT)y1;
  corners[1].x=(FLOAT)x2;
  corners[1].y=(FLOAT)y2;
  pointStart.x=(FLOAT)xStart;
  pointStart.y=(FLOAT)yStart;
  pointEnd.x=(FLOAT)xEnd;
  pointEnd.y=(FLOAT)yEnd;
  INTERNAL_LPTODP_FLOAT(pDC, corners);
  INTERNAL_LPTODP_FLOAT(pDC, corners+1);
  INTERNAL_LPTODP_FLOAT(pDC, &pointStart);
  INTERNAL_LPTODP_FLOAT(pDC, &pointEnd);

  /* Make sure first corner is top left and second corner is bottom right */
  if ( corners[0].x > corners[1].x )
  {
    temp=corners[0].x;
    corners[0].x=corners[1].x;
    corners[1].x=temp;
  }
  if ( corners[0].y > corners[1].y )
  {
    temp=corners[0].y;
    corners[0].y=corners[1].y;
    corners[1].y=temp;
  }

  /* Compute start and end angle */
  PATH_NormalizePoint(corners, &pointStart, &x, &y);
  angleStart=atan2(y, x);
  PATH_NormalizePoint(corners, &pointEnd, &x, &y);
  angleEnd=atan2(y, x);

  /* Make sure the end angle is "on the right side" of the start angle */
  if ( clockwise )
  {
    if ( angleEnd <= angleStart )
    {
      angleEnd+=2*M_PI;
      assert(angleEnd>=angleStart);
    }
  }
  else
  {
    if(angleEnd>=angleStart)
    {
      angleEnd-=2*M_PI;
      assert(angleEnd<=angleStart);
    }
  }

  /* In GM_COMPATIBLE, don't include bottom and right edges */
  if ( IntGetGraphicsMode(dc) == GM_COMPATIBLE )
  {
    corners[1].x--;
    corners[1].y--;
  }

  /* Add the arc to the path with one Bezier spline per quadrant that the
   * arc spans */
  start=TRUE;
  end=FALSE;
  do
  {
    /* Determine the start and end angles for this quadrant */
    if(start)
    {
      angleStartQuadrant=angleStart;
      if ( clockwise )
        angleEndQuadrant=(floor(angleStart/M_PI_2)+1.0)*M_PI_2;
      else
        angleEndQuadrant=(ceil(angleStart/M_PI_2)-1.0)*M_PI_2;
    }
    else
    {
      angleStartQuadrant=angleEndQuadrant;
      if ( clockwise )
        angleEndQuadrant+=M_PI_2;
      else
        angleEndQuadrant-=M_PI_2;
    }

    /* Have we reached the last part of the arc? */
    if ( (clockwise && angleEnd<angleEndQuadrant)
      || (!clockwise && angleEnd>angleEndQuadrant)
      )
    {
      /* Adjust the end angle for this quadrant */
     angleEndQuadrant = angleEnd;
     end = TRUE;
    }

    /* Add the Bezier spline to the path */
    PATH_DoArcPart ( pPath, corners, angleStartQuadrant, angleEndQuadrant, start );
    start = FALSE;
  } while(!end);

  return TRUE;
}

BOOL
FASTCALL
PATH_PolyBezierTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  GdiPath *pPath;
  POINT pt;
  ULONG i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke=FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(pPath, &pt, PT_MOVETO) )
        return FALSE;
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, PT_BEZIERTO);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyBezier ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  GdiPath *pPath;
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  PATH_GetPathFromDC ( dc, &pPath );

   /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry ( pPath, &pt, (i == 0) ? PT_MOVETO : PT_BEZIERTO );
  }

  return TRUE;
}

BOOL
FASTCALL
PATH_Polyline ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  GdiPath *pPath;
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  for ( i = 0; i < cbPoints; i++ )
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO : PT_LINETO);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolylineTo ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  GdiPath *pPath;
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( cbPoints );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Add a PT_MOVETO if necessary */
  if ( pPath->newStroke )
  {
    pPath->newStroke = FALSE;
    IntGetCurrentPositionEx ( dc, &pt );
    CoordLPtoDP ( dc, &pt );
    if ( !PATH_AddEntry(pPath, &pt, PT_MOVETO) )
      return FALSE;
  }

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, PT_LINETO);
  }

  return TRUE;
}


BOOL
FASTCALL
PATH_Polygon ( PDC dc, const POINT *pts, DWORD cbPoints )
{
  GdiPath *pPath;
  POINT   pt;
  ULONG   i;

  ASSERT ( dc );
  ASSERT ( pts );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  for(i = 0; i < cbPoints; i++)
  {
    pt = pts[i];
    CoordLPtoDP ( dc, &pt );
    PATH_AddEntry(pPath, &pt, (i == 0) ? PT_MOVETO :
      ((i == cbPoints-1) ? PT_LINETO | PT_CLOSEFIGURE :
      PT_LINETO));
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons )
{
  GdiPath *pPath;
  POINT   pt, startpt;
  ULONG   poly, point, i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polygons );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open );
    return FALSE;

  for(i = 0, poly = 0; poly < polygons; poly++)
  {
    for(point = 0; point < (ULONG) counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      if(point == 0) startpt = pt;
        PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
    /* win98 adds an extra line to close the figure for some reason */
    PATH_AddEntry(pPath, &startpt, PT_LINETO | PT_CLOSEFIGURE);
  }
  return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolyline ( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines )
{
  GdiPath *pPath;
  POINT   pt;
  ULONG   poly, point, i;

  ASSERT ( dc );
  ASSERT ( pts );
  ASSERT ( counts );
  ASSERT ( polylines );

  PATH_GetPathFromDC ( dc, &pPath );

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  for(i = 0, poly = 0; poly < polylines; poly++)
  {
    for(point = 0; point < counts[poly]; point++, i++)
    {
      pt = pts[i];
      CoordLPtoDP ( dc, &pt );
      PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
    }
  }
  return TRUE;
}

/***********************************************************************
 * Internal functions
 */


/* PATH_AddFlatBezier
 *
 */
BOOL
FASTCALL
PATH_AddFlatBezier ( GdiPath *pPath, POINT *pt, BOOL closed )
{
  POINT *pts;
  INT no, i;

  pts = GDI_Bezier( pt, 4, &no );
  if ( !pts ) return FALSE;

  for(i = 1; i < no; i++)
    PATH_AddEntry(pPath, &pts[i],  (i == no-1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO);

  ExFreePool(pts);
  return TRUE;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
BOOL
FASTCALL
PATH_FlattenPath(GdiPath *pPath)
{
  GdiPath newPath;
  INT srcpt;

  memset(&newPath, 0, sizeof(newPath));
  newPath.state = PATH_Open;
  for(srcpt = 0; srcpt < pPath->numEntriesUsed; srcpt++) {
    switch(pPath->pFlags[srcpt] & ~PT_CLOSEFIGURE) {
      case PT_MOVETO:
      case PT_LINETO:
        PATH_AddEntry(&newPath, &pPath->pPoints[srcpt], pPath->pFlags[srcpt]);
        break;
      case PT_BEZIERTO:
        PATH_AddFlatBezier(&newPath, &pPath->pPoints[srcpt-1], pPath->pFlags[srcpt+2] & PT_CLOSEFIGURE);
        srcpt += 2;
        break;
    }
  }
  newPath.state = PATH_Closed;
  PATH_AssignGdiPath(pPath, &newPath);
  PATH_EmptyPath(&newPath);
  return TRUE;
}

/* PATH_PathToRegion
 *
 * Creates a region from the specified path using the specified polygon
 * filling mode. The path is left unchanged. A handle to the region that
 * was created is stored in *pHrgn. If successful, TRUE is returned; if an
 * error occurs, SetLastError is called with the appropriate value and
 * FALSE is returned.
 */
#if 0
// FIXME - don't reenable this function until you deal with the
// const pPath being given to PATH_FlattenPath() - which is
// expecting a non-const*. Since this function isn't being called
// at the moment, I'm commenting it out until the issue needs to
// be addressed.
BOOL 
FASTCALL
PATH_PathToRegion ( const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn )
{
  int    numStrokes, iStroke, i;
  INT  *pNumPointsInStroke;
  HRGN hrgn;

  assert ( pPath!=NULL );
  assert ( pHrgn!=NULL );

  PATH_FlattenPath ( pPath );

  /* FIXME: What happens when number of points is zero? */

  /* First pass: Find out how many strokes there are in the path */
  /* FIXME: We could eliminate this with some bookkeeping in GdiPath */
  numStrokes=0;
  for(i=0; i<pPath->numEntriesUsed; i++)
    if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
      numStrokes++;

  /* Allocate memory for number-of-points-in-stroke array */
  pNumPointsInStroke=(int *)ExAllocatePoolWithTag(NonPagedPool, sizeof(int) * numStrokes, TAG_PATH);
  if(!pNumPointsInStroke)
  {
//    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* Second pass: remember number of points in each polygon */
  iStroke=-1;  /* Will get incremented to 0 at beginning of first stroke */
  for(i=0; i<pPath->numEntriesUsed; i++)
  {
    /* Is this the beginning of a new stroke? */
    if((pPath->pFlags[i] & ~PT_CLOSEFIGURE) == PT_MOVETO)
    {
      iStroke++;
      pNumPointsInStroke[iStroke]=0;
    }

    pNumPointsInStroke[iStroke]++;
  }

  /* Create a region from the strokes */
/*  hrgn=CreatePolyPolygonRgn(pPath->pPoints, pNumPointsInStroke,
    numStrokes, nPolyFillMode); FIXME: reinclude when region code implemented */
  if(hrgn==(HRGN)0)
  {
//    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  /* Free memory for number-of-points-in-stroke array */
  ExFreePool(pNumPointsInStroke);

  /* Success! */
  *pHrgn=hrgn;
  return TRUE;
}
#endif

/* PATH_EmptyPath
 *
 * Removes all entries from the path and sets the path state to PATH_Null.
 */
VOID
FASTCALL
PATH_EmptyPath ( GdiPath *pPath )
{
  assert(pPath!=NULL);

  pPath->state=PATH_Null;
  pPath->numEntriesUsed=0;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
BOOL
FASTCALL
PATH_AddEntry ( GdiPath *pPath, const POINT *pPoint, BYTE flags )
{
  assert(pPath!=NULL);

  /* FIXME: If newStroke is true, perhaps we want to check that we're
   * getting a PT_MOVETO
   */

  /* Check that path is open */
  if ( pPath->state != PATH_Open )
    return FALSE;

  /* Reserve enough memory for an extra path entry */
  if ( !PATH_ReserveEntries(pPath, pPath->numEntriesUsed+1) )
    return FALSE;

  /* Store information in path entry */
  pPath->pPoints[pPath->numEntriesUsed]=*pPoint;
  pPath->pFlags[pPath->numEntriesUsed]=flags;

  /* If this is PT_CLOSEFIGURE, we have to start a new stroke next time */
  if((flags & PT_CLOSEFIGURE) == PT_CLOSEFIGURE)
    pPath->newStroke=TRUE;

  /* Increment entry count */
  pPath->numEntriesUsed++;

  return TRUE;
}

/* PATH_ReserveEntries
 *
 * Ensures that at least "numEntries" entries (for points and flags) have
 * been allocated; allocates larger arrays and copies the existing entries
 * to those arrays, if necessary. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_ReserveEntries ( GdiPath *pPath, INT numEntries )
{
  INT   numEntriesToAllocate;
  POINT *pPointsNew;
  BYTE    *pFlagsNew;

  assert(pPath!=NULL);
  assert(numEntries>=0);

  /* Do we have to allocate more memory? */
  if(numEntries > pPath->numEntriesAllocated)
  {
    /* Find number of entries to allocate. We let the size of the array
     * grow exponentially, since that will guarantee linear time
     * complexity. */
    if(pPath->numEntriesAllocated)
    {
      numEntriesToAllocate=pPath->numEntriesAllocated;
      while(numEntriesToAllocate<numEntries)
        numEntriesToAllocate=numEntriesToAllocate*GROW_FACTOR_NUMER/GROW_FACTOR_DENOM;
    } else
       numEntriesToAllocate=numEntries;

    /* Allocate new arrays */
    pPointsNew=(POINT *)ExAllocatePoolWithTag(NonPagedPool, numEntriesToAllocate * sizeof(POINT), TAG_PATH);
    if(!pPointsNew)
      return FALSE;
    pFlagsNew=(BYTE *)ExAllocatePoolWithTag(NonPagedPool, numEntriesToAllocate * sizeof(BYTE), TAG_PATH);
    if(!pFlagsNew)
    {
      ExFreePool(pPointsNew);
      return FALSE;
    }

    /* Copy old arrays to new arrays and discard old arrays */
    if(pPath->pPoints)
    {
      assert(pPath->pFlags);

      memcpy(pPointsNew, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
      memcpy(pFlagsNew, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

      ExFreePool(pPath->pPoints);
      ExFreePool(pPath->pFlags);
    }
    pPath->pPoints=pPointsNew;
    pPath->pFlags=pFlagsNew;
    pPath->numEntriesAllocated=numEntriesToAllocate;
  }

  return TRUE;
}

/* PATH_GetPathFromDC
 *
 * Retrieves a pointer to the GdiPath structure contained in an HDC and
 * places it in *ppPath. TRUE is returned if successful, FALSE otherwise.
 */
VOID
FASTCALL
PATH_GetPathFromDC ( PDC dc, GdiPath **ppPath )
{
  ASSERT ( dc );
  ASSERT ( ppPath );
  *ppPath = &dc->w.path;
}

/* PATH_DoArcPart
 *
 * Creates a Bezier spline that corresponds to part of an arc and appends the
 * corresponding points to the path. The start and end angles are passed in
 * "angleStart" and "angleEnd"; these angles should span a quarter circle
 * at most. If "addMoveTo" is true, a PT_MOVETO entry for the first control
 * point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
BOOL
FASTCALL
PATH_DoArcPart ( GdiPath *pPath, FLOAT_POINT corners[],
   double angleStart, double angleEnd, BOOL addMoveTo )
{
  double  halfAngle, a;
  double  xNorm[4], yNorm[4];
  POINT point;
  int i;

  assert(fabs(angleEnd-angleStart)<=M_PI_2);

  /* FIXME: Is there an easier way of computing this? */

  /* Compute control points */
  halfAngle=(angleEnd-angleStart)/2.0;
  if(fabs(halfAngle)>1e-8)
  {
    a=4.0/3.0*(1-cos(halfAngle))/sin(halfAngle);
    xNorm[0]=cos(angleStart);
    yNorm[0]=sin(angleStart);
    xNorm[1]=xNorm[0] - a*yNorm[0];
    yNorm[1]=yNorm[0] + a*xNorm[0];
    xNorm[3]=cos(angleEnd);
    yNorm[3]=sin(angleEnd);
    xNorm[2]=xNorm[3] + a*yNorm[3];
    yNorm[2]=yNorm[3] - a*xNorm[3];
  } else
    for(i=0; i<4; i++)
    {
      xNorm[i]=cos(angleStart);
      yNorm[i]=sin(angleStart);
    }

  /* Add starting point to path if desired */
  if(addMoveTo)
  {
    PATH_ScaleNormalizedPoint(corners, xNorm[0], yNorm[0], &point);
    if(!PATH_AddEntry(pPath, &point, PT_MOVETO))
      return FALSE;
  }

  /* Add remaining control points */
  for(i=1; i<4; i++)
  {
    PATH_ScaleNormalizedPoint(corners, xNorm[i], yNorm[i], &point);
    if(!PATH_AddEntry(pPath, &point, PT_BEZIERTO))
      return FALSE;
  }

  return TRUE;
}

/* PATH_ScaleNormalizedPoint
 *
 * Scales a normalized point (x, y) with respect to the box whose corners are
 * passed in "corners". The point is stored in "*pPoint". The normalized
 * coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
 * (1.0, 1.0) correspond to corners[1].
 */
VOID
FASTCALL
PATH_ScaleNormalizedPoint ( FLOAT_POINT corners[], double x,
   double y, POINT *pPoint )
{
  ASSERT ( corners );
  ASSERT ( pPoint );
  pPoint->x=GDI_ROUND( (double)corners[0].x + (double)(corners[1].x-corners[0].x)*0.5*(x+1.0) );
  pPoint->y=GDI_ROUND( (double)corners[0].y + (double)(corners[1].y-corners[0].y)*0.5*(y+1.0) );
}

/* PATH_NormalizePoint
 *
 * Normalizes a point with respect to the box whose corners are passed in
 * corners. The normalized coordinates are stored in *pX and *pY.
 */
VOID
FASTCALL
PATH_NormalizePoint ( FLOAT_POINT corners[],
   const FLOAT_POINT *pPoint,
   double *pX, double *pY)
{
  ASSERT ( corners );
  ASSERT ( pPoint );
  ASSERT ( pX );
  ASSERT ( pY );
  *pX=(double)(pPoint->x-corners[0].x)/(double)(corners[1].x-corners[0].x) * 2.0 - 1.0;
  *pY=(double)(pPoint->y-corners[0].y)/(double)(corners[1].y-corners[0].y) * 2.0 - 1.0;
}
/* EOF */
