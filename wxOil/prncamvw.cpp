// $Id$
/* @@tag:xara-cn@@ DO NOT MODIFY THIS LINE
================================XARAHEADERSTART===========================
 
               Xara LX, a vector drawing and manipulation program.
                    Copyright (C) 1993-2006 Xara Group Ltd.
       Copyright on certain contributions may be held in joint with their
              respective authors. See AUTHORS file for details.

LICENSE TO USE AND MODIFY SOFTWARE
----------------------------------

This file is part of Xara LX.

Xara LX is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as published
by the Free Software Foundation.

Xara LX and its component source files are distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with Xara LX (see the file GPL in the root directory of the
distribution); if not, write to the Free Software Foundation, Inc., 51
Franklin St, Fifth Floor, Boston, MA  02110-1301 USA


ADDITIONAL RIGHTS
-----------------

Conditional upon your continuing compliance with the GNU General Public
License described above, Xara Group Ltd grants to you certain additional
rights. 

The additional rights are to use, modify, and distribute the software
together with the wxWidgets library, the wxXtra library, and the "CDraw"
library and any other such library that any version of Xara LX relased
by Xara Group Ltd requires in order to compile and execute, including
the static linking of that library to XaraLX. In the case of the
"CDraw" library, you may satisfy obligation under the GNU General Public
License to provide source code by providing a binary copy of the library
concerned and a copy of the license accompanying it.

Nothing in this section restricts any of the rights you have under
the GNU General Public License.


SCOPE OF LICENSE
----------------

This license applies to this program (XaraLX) and its constituent source
files only, and does not necessarily apply to other Xara products which may
in part share the same code base, and are subject to their own licensing
terms.

This license does not apply to files in the wxXtra directory, which
are built into a separate library, and are subject to the wxWindows
license contained within that directory in the file "WXXTRA-LICENSE".

This license does not apply to the binary libraries (if any) within
the "libs" directory, which are subject to a separate license contained
within that directory in the file "LIBS-LICENSE".


ARRANGEMENTS FOR CONTRIBUTION OF MODIFICATIONS
----------------------------------------------

Subject to the terms of the GNU Public License (see above), you are
free to do whatever you like with your modifications. However, you may
(at your option) wish contribute them to Xara's source tree. You can
find details of how to do this at:
  http://www.xaraxtreme.org/developers/

Prior to contributing your modifications, you will need to complete our
contributor agreement. This can be found at:
  http://www.xaraxtreme.org/developers/contribute/

Please note that Xara will not accept modifications which modify any of
the text between the start and end of this header (marked
XARAHEADERSTART and XARAHEADEREND).


MARKS
-----

Xara, Xara LX, Xara X, Xara X/Xtreme, Xara Xtreme, the Xtreme and Xara
designs are registered or unregistered trademarks, design-marks, and/or
service marks of Xara Group Ltd. All rights in these marks are reserved.


      Xara Group Ltd, Gaddesden Place, Hemel Hempstead, HP2 6EX, UK.
                        http://www.xara.com/

=================================XARAHEADEREND============================
 */

// PrintCamView - subclass of CamView used for printing.

/*
*/

#include "camtypes.h"

#include "prncamvw.h"

#include "camdoc.h"
//#include "document.h" - in camtypes.h [AUTOMATICALLY REMOVED]
#include "prntview.h"
//#include "docview.h" - in camtypes.h [AUTOMATICALLY REMOVED]
//#include "spread.h" - in camtypes.h [AUTOMATICALLY REMOVED]
//#include "printdlg.h"
#include "prdlgctl.h"
#include "princomp.h"
#include "printctl.h"
#include "psrndrgn.h"
//#include "tim.h"
#include "camelot.h"
//#include "resource.h"
//#include "app.h" - in camtypes.h [AUTOMATICALLY REMOVED]
#include "prnmks.h"
#include "opbevel.h"

DECLARE_SOURCE("$Revision$");

#define new CAM_DEBUG_NEW
#define USERNAME "Mike"

BOOL PrintMonitor::StartPrintJob(CNativeDC * PrintJobDC)
{
	// Make sure we don't already have a print job going.
	if (CurrentPrintJob != NULL)
	{
PORTNOTE("printing", "Don't call ::AbortDoc")
#ifndef EXCLUDE_FROM_XARALX
		// Print job going already - must have been terminated early if the
		// user has initiated another print job, so clean it up first.
		if (::AbortDoc(CurrentPrintJob) <= 0)
		{
			// Could not abort print job.
			CurrentPrintJob = NULL;
			ERROR1(FALSE, _R(IDE_ALREADY_PRINTING));
		}
#endif
	}

	// Ok, starting a print job - keep a record of it.
	CurrentPrintJob = PrintJobDC;
	return TRUE;
}

void PrintMonitor::EndPrintJob()
{
	// Ok, throw away our reference to the print job.
	CurrentPrintJob = NULL;
}

BOOL PrintMonitor::IsPrintStatusOK()
{
PORTNOTE("printing", "Disabled IsPrintStatusOK()")
#ifndef EXCLUDE_FROM_XARALX
	if (CCamApp::SeriousErrorCount == MySeriousErrorCount)
	{
		// We haven't had a serious error so any current print job will be
		// proceeding normally.
		return TRUE;
	}

	// Update our error count so we get back in sync.
	MySeriousErrorCount = CCamApp::SeriousErrorCount;

	BOOL Result = TRUE;

	// An error has occured - delete the print job if necessary.
	if (CurrentPrintJob != NULL)
	{
		// Use a semaphore structure here so we don't do this more than once.
		CNativeDC * PrintDC = CurrentPrintJob;
		CurrentPrintJob = NULL;
		::AbortDoc(PrintDC);

		// Something's gone wrong - tell caller to remove all printing render regions.
		Result = FALSE;
	}
//	WEBSTER-ranbirr-12/11/96
#ifndef WEBSTER
	CCPrintInfo *pPrintInfo = CCPrintInfo::GetCurrent();
	if (pPrintInfo != NULL)
	{
		// Close down and delete this print info object (this will delete the print progress 
		// dialog etc.)
		pPrintInfo->EndPrinting();
		delete pPrintInfo;

		// Something's gone wrong - tell caller to remove all printing render regions.
		Result = FALSE;
	}
#endif //webster
	// Re-enable print Operation, and return result to caller.
	SetPrintingActive(FALSE);
#endif
	return TRUE;
}

void PrintMonitor::SetPrintingActive(BOOL Flag)
{
	PrintingIsActive = Flag;
}

BOOL PrintMonitor::IsPrintingActive()
{
	return (PrintingIsActive);
}


/********************************************************************************************

>	BOOL PrintMonitor::IsPrintingNow()

	Author:		Rik_Heywood (Xara Group Ltd) <camelotdev@xara.com>
	Created:	3/8/95
	Returns:	TRUE if we are activly printing NOW, FALSE if not
	Purpose:	Allows you to determine if there is a currently active print happening. This
				is used to disable screen redrawing, only during the print itself.

********************************************************************************************/

BOOL PrintMonitor::IsPrintingNow()
{
	// if there is no print job, then we are not printing
	if (CurrentPrintJob==NULL)
		return FALSE;

	// otherwise we are printing
	return TRUE;
}



/********************************************************************************************

>	void PrintMonitor::WantFullRedraw(BOOL WantRedraw)

	Author:		Rik_Heywood (Xara Group Ltd) <camelotdev@xara.com>
	Created:	3/8/95
	Inputs:		WantRedraw - TRUE if we want to force a redraw, FALSE if not
	Purpose:	Use this function to tell the PrintMonitor that a Full Redraw will be needed
				at the end of the print job. You can also use this function to tell it
				that the Full Redraw has been done.

********************************************************************************************/

void PrintMonitor::WantFullRedraw(BOOL WantRedraw)
{
	FullRedrawNeeded = WantRedraw;
}


BOOL PrintMonitor::InitPrefs()
{
	// Declare and load the .INI file settings.
	BOOL ok = Camelot.DeclareSection(TEXT("Printing"), 10) &&
		   	  Camelot.DeclarePref(TEXT("Printing"), TEXT("PrintMaskType"),
		   					   	  (INT32 *) &PrintMonitor::PrintMaskType, MASK_SIMPLE, MASK_OPTIMAL) &&
		   	  Camelot.DeclarePref(TEXT("Printing"), TEXT("PrintWithDriverBands"),
		   					   	  &PrintMonitor::PrintWithDriverBands, FALSE, TRUE);
	return ok;
}


// No current print job.
CNativeDC * PrintMonitor::CurrentPrintJob = NULL;
BOOL PrintMonitor::PrintingIsActive = FALSE;
BOOL PrintMonitor::FullRedrawNeeded = FALSE;

// No errors as yet.
INT32 PrintMonitor::MySeriousErrorCount = 0;

// Default to printing without mask, but with bands.
BOOL PrintMonitor::PrintWithDriverBands = TRUE;
BOOL PrintMonitor::PrintWithMask = TRUE;
PrintMonitor::MaskType PrintMonitor::PrintMaskType = MASK_OPTIMAL;


