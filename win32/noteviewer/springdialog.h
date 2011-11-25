/*
	GUIDO Library
	Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#if !defined(AFX_SPRINGDIALOG_H__36DEDDE0_D2FD_11D2_AD0A_0080C75E70DF__INCLUDED_)
#define AFX_SPRINGDIALOG_H__36DEDDE0_D2FD_11D2_AD0A_0080C75E70DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"
#include "GUIDOEngine.h"

/////////////////////////////////////////////////////////////////////////////
// Dialogbox CSpringDialog 

class CSpringDialog : public CDialog
{
// Konstruktion
public:
	float springpar;
	CSpringDialog( GuidoLayoutSettings * settings, CWnd * pParent );   // Standardconstructor

// dialogboxdata
	//{{AFX_DATA(CSpringDialog)
	enum { IDD = IDD_SPRINGDIALOG };
	CSliderCtrl	m_slider;
	CEdit	m_EditSpring;
	//}}AFX_DATA


// overrides
	// virtual overrides generated by the class wizzar
	//{{AFX_VIRTUAL(CSpringDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-support
	//}}AFX_VIRTUAL

// implementation
protected:

	GuidoLayoutSettings	* mSettings;

	// generated message maps 	//{{AFX_MSG(CSpringDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// MS Visual C++ inserts directly before the previous line additional deklarations

#endif // AFX_SPRINGDIALOG_H__36DEDDE0_D2FD_11D2_AD0A_0080C75E70DF__INCLUDED_
