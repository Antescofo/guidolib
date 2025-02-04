/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2004 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include <iostream>
#include <string.h>
#include <cctype>

#include "ARHarmony.h"
#include "GRHarmony.h"

#include "GRDefine.h"
#include "FontManager.h"
#include "GRPage.h"
#include "GRRod.h"
#include "GRStaff.h"
#include "GRText.h"
#include "GUIDOInternal.h"	// for gGlobalSettings.gDevice

#include "TagParameterFloat.h"
#include "VGDevice.h"
#include "VGFont.h"
#include "FontManager.h"

using namespace std;

#ifndef WIN32
#warning ("TODO: set bounding box (currently ignored)");
#endif


extern GRStaff * gCurStaff;

GRHarmony::GRHarmony(GRStaff * p_staff, const ARHarmony * ar)
  : GRPTagARNotationElement(ar)
{
	assert(ar);

	GRSystemStartEndStruct * sse = new GRSystemStartEndStruct;
	GRTextSaveStruct * st = new GRTextSaveStruct;
	sse->p = (void *) st;
	sse->grsystem = p_staff->getGRSystem();
	sse->startflag = GRSystemStartEndStruct::LEFTMOST;

	mMustFollowPitch = false;
	addSystemStartEndStruct (sse);

	float curLSPACE = p_staff ? p_staff->getStaffLSPACE() : LSPACE;

    if( mMustFollowPitch == false ) {
        int pos = ar->position();
        if (p_staff) {
            switch (pos) {
                case ARHarmony::kUndefined:
                case ARHarmony::kBelow:
                    mPosition.y = p_staff->getDredgeSize(); // + curLSPACE;
                    break;
                case ARHarmony::kAbove:
                    mPosition.y = -p_staff->getDredgeSize() - curLSPACE/2;
                    break;
            }
        }
        else mPosition.y = -curLSPACE;
    }

	if (ar) st->text = ar->getText() ? ar->getText() : "";
	
	mTextAlign = VGDevice::kAlignLeft + VGDevice::kAlignTop;
	fFont = FontManager::GetTextFont(ar, curLSPACE, mTextAlign);


	float sizex = 0;
	float sizey = 0;
	if( gGlobalSettings.gDevice)
		fFont->GetExtent( st->text.c_str(), (int)st->text.size(), &sizex, &sizey, gGlobalSettings.gDevice );

    st->boundingBox.right = 0; //sizex;
	st->boundingBox.top = sizey;
	st->boundingBox.bottom = 4 * LSPACE;

    mBoundingBox.left = 0;
    mBoundingBox.right = sizex;
    mBoundingBox.top = 0;
    mBoundingBox.bottom = sizey;
}


GRHarmony::~GRHarmony()
{
	assert(mStartEndList.empty());
	// this is important ...
	// All associaions that have been made are dealt with in GRPositionTag ...

	// this makes sure, that we don't remove associations, that are no longer there
	// after the Tag has been delete (especially, if more than one system is handled.)
	delete mAssociated;
	mAssociated = 0;
}

void GRHarmony::OnDraw( VGDevice & hdc ) const
{
	if(!mDraw || !mShow)
		return;
	GRSystemStartEndStruct * sse = getSystemStartEndStruct( gCurSystem );
	assert(sse);
	GRTextSaveStruct * st = (GRTextSaveStruct *) sse->p;

	const ARHarmony * arText = getARHarmony();
	const float curLSPACE = gCurStaff ? gCurStaff->getStaffLSPACE(): LSPACE;

	// - Setup position.
	// y-reference position if the lowest line of the staff.
	NVPoint drawPos (st->position);

	// - Force the position to be relative to the bottom line of the staff
	if( mMustFollowPitch == false ) {
		int pos = arText->position();
		if (gCurStaff) {
			switch (pos) {
				case ARHarmony::kUndefined:
				case ARHarmony::kBelow:
					drawPos.y = gCurStaff->getDredgeSize(); // + curLSPACE;
					break;
				case ARHarmony::kAbove:
					drawPos.y = -gCurStaff->getDredgeSize() - curLSPACE/2;
					break;
			}
		}
		else drawPos.y = -curLSPACE;
	}
	float dx = arText->getDX()->getValue( curLSPACE );
	float dy = -arText->getDY()->getValue( curLSPACE );;

	hdc.SetTextFont( fFont);
	const VGColor prevTextColor = hdc.GetFontColor();
	if( mColRef ) hdc.SetFontColor( VGColor( mColRef ));
	hdc.SetFontAlign( mTextAlign );
	DrawHarmonyString (hdc, fFont, st->text, drawPos.x + st->boundingBox.left + dx, drawPos.y + dy);

	if( mColRef ) hdc.SetFontColor( prevTextColor );
//	DrawBoundingBox(hdc, kEventBBColor);
}

float GRHarmony::CharExtend (const char* c, const VGFont* font, VGDevice* hdc) const
{
	float w, h;
	font->GetExtent (c, 1, &w, &h, hdc);
	return w;
}

void GRHarmony::DrawHarmonyString (VGDevice & hdc, const VGFont* font, const string& str, float x, float y) const
{
	if (str.empty()) return;

	const VGFont* mfont = hdc.GetMusicFont();
	if (!mfont) mfont = FontManager::gFontScriab;
	
	float ratio = font->GetSize() / 150.f; // 150 is the default font size for harmony (20 pt)
	int fsize = mfont->GetSize() * ratio;

	const VGFont* mBigFont = FontManager::FindOrCreateFont( int(fsize * 1.0), mfont->GetName(), "");
	const VGFont* mSmallFont = FontManager::FindOrCreateFont( int(fsize * 0.8), mfont->GetName(), "");
	const VGFont* tSmallFont = FontManager::FindOrCreateFont( int(font->GetSize() * 0.8), font->GetName(), "");
	const VGFont* curmfont = mBigFont;
	const VGFont* curtfont = font;
	const char * ptr = str.c_str();
	float moffset = 2 * LSPACE * ratio;
	float yoffset = 0;
	float xoffset = LSPACE/2;
	bool inSecPart = false;
    bool isBase = false;

	hdc.SetMusicFont (mBigFont);
	while (*ptr) {
		char c = *ptr++;
		char buff[2] = { c, 0 };
		if (!inSecPart) {
			inSecPart = std::isdigit(c);
			if (inSecPart) {
				hdc.SetMusicFont (mSmallFont);
				moffset = 1.5 * LSPACE * ratio;
				yoffset = -2;
				xoffset /= 2;
			}
		}
        
        if (!isBase) {
            isBase = (c == '/');
            if (isBase) {
                hdc.SetTextFont (font);
                curtfont = font;
                yoffset = 0;
                xoffset = LSPACE/2;
                if (inSecPart) {
                    inSecPart = false;
                    hdc.SetMusicFont (mBigFont);
                    moffset = 2 * LSPACE * ratio;
                }
            }
        }

		if (c == '#') {
			hdc.DrawMusicSymbol(x + 10, y - moffset - LSPACE/2, kSharpSymbol);
			buff[0] = kSharpSymbol;
			x += CharExtend (buff, curmfont, &hdc) + xoffset;
		}
		else if (c == '&') {
			hdc.DrawMusicSymbol(x + 10, y - moffset - (inSecPart ? LSPACE/2: 0), kFlatSymbol);
			buff[0] = kFlatSymbol;
			x += CharExtend (buff, curmfont, &hdc) + xoffset;
		}
		else {
			hdc.DrawString( x, y + yoffset, buff, 1);
			x += CharExtend (buff, curtfont, &hdc);
            if (isBase) {
                if (c=='/') continue;
            }
			hdc.SetTextFont (tSmallFont);
			if (!inSecPart) {
				curtfont = tSmallFont;
				yoffset = float(font->GetSize() - curtfont->GetSize());
			}
		}
	}
	hdc.SetMusicFont (mfont);
}

const ARHarmony * GRHarmony::getARHarmony() const
{
	return static_cast<const ARHarmony*>(getAbstractRepresentation());
}

void GRHarmony::addAssociation(GRNotationElement * el)
{
	GRNotationElement::addAssociation(el);
	GRPositionTag::addAssociation(el);
}
/** \brief Called directly by a spring. Then we know that we
	do not have a position tag.
*/
void GRHarmony::setPosition(const NVPoint & inPosition)
{
	GRPTagARNotationElement::setPosition(inPosition);

	// there can be only one sse! -> no overlap
	assert(mStartEndList.size() == 1);
	GRSystemStartEndStruct * sse = mStartEndList.GetHead();

	GRTextSaveStruct * st = (GRTextSaveStruct *) sse->p;
	assert(st);
	st->position = inPosition;
}

void GRHarmony::setHPosition( GCoord nx )
{
	GRPTagARNotationElement::setHPosition(nx);
	// there can be only one sse! -> no overlap
	assert(mStartEndList.size() == 1);
	GRSystemStartEndStruct * sse = mStartEndList.GetHead();

	GRTextSaveStruct * st = (GRTextSaveStruct *) sse->p;
	assert(st);
	st->position.x = nx;
	mPosition.x = nx;
}

void GRHarmony::tellPosition(GObject * caller, const NVPoint & inPosition)
{
	// this can be only called by an event, that is there ..
	GRNotationElement * grel =  dynamic_cast<GRNotationElement *>(caller);
	if( grel == 0 ) return;

	GRStaff * staff = grel->getGRStaff();
	if( staff == 0 ) return;

	GRSystemStartEndStruct * sse = getSystemStartEndStruct(staff->getGRSystem());
	assert(sse);

	GRTextSaveStruct * st = (GRTextSaveStruct *)sse->p;
	GRNotationElement * startElement = sse->startElement;
	NVPoint newPos( inPosition );

	// - Check if we're left-opened
	if (sse->startflag == GRSystemStartEndStruct::OPENLEFT)
	{
		if (grel != startElement)
		{
			if (st->position.x == 0)
			{
				newPos.x -= LSPACE * 0.5f; // this is actually notebreite!
				st->position = newPos;
				st->text = "-";
			}
		}
	}

	// - Follows the y-position of the first element of the range (if any)
	else if (grel == startElement)
	{
		newPos.y = grel->getPosition().y;
		st->position = newPos;

//		const ARHarmony* arText = getARHarmony();
//		const char* text = arText ? arText->getText() : 0;
//		if (text) st->text = text;
        
        FloatRect r = getTextMetrics (*gGlobalSettings.gDevice, staff);
        setPosition (NVPoint(r.left, r.top));
        NVRect bb (0, 0, r.Width(), r.Height());
        mBoundingBox = bb;
        float xoffset = 0;
        if (mTextAlign & VGDevice::kAlignCenter)
            xoffset = mBoundingBox.Width()/2;
        if (mTextAlign & VGDevice::kAlignRight)
            xoffset = mBoundingBox.Width();
        mBoundingBox -= NVPoint(xoffset, 0);
	}
}

FloatRect GRHarmony::getTextMetrics(VGDevice & hdc, const GRStaff* staff ) const
{
    FloatRect r;

    GRSystemStartEndStruct * sse = getSystemStartEndStruct( staff->getGRSystem() );
    assert(sse);
    GRTextSaveStruct * st = (GRTextSaveStruct *) sse->p;
    const ARHarmony * arText = getARHarmony();
    const float curLSPACE = staff ? staff->getStaffLSPACE(): LSPACE;
    // - Setup position.
    // y-reference position if the lowest line of the staff.
    NVPoint drawPos (st->position);
    if( mMustFollowPitch == false ) {
        int pos = arText->position();
        if (staff) {
            switch (pos) {
                case ARHarmony::kUndefined:
                case ARHarmony::kBelow:
                    drawPos.y = staff->getDredgeSize(); // + curLSPACE;
                    break;
                case ARHarmony::kAbove:
                    drawPos.y = -staff->getDredgeSize() - curLSPACE/2;
                    break;
            }
        }
        else drawPos.y = -curLSPACE;
    }

    float dx = arText->getDX()->getValue( curLSPACE );
    float dy = -arText->getDY()->getValue( curLSPACE );

    float x = drawPos.x + st->boundingBox.left + dx;
    float y = drawPos.y + dy;
    float w = 0;
    float h = 0;

    const VGFont* savedFont = hdc.GetTextFont();
    hdc.SetTextFont( fFont );
    fFont->GetExtent(st->text.c_str(), (int)st->text.size(), &w, &h, &hdc);
    hdc.SetTextFont( savedFont );
    
    r.Set(x, y, x+w, y+h);
    return r;
}

void GRHarmony::accept (GRVisitor& visitor)
{
    visitor.visitStart (this);
    visitor.visitEnd (this);
}

void GRHarmony::removeAssociation(GRNotationElement * el)
{
	GRPositionTag::removeAssociation(el);
	GRARNotationElement::removeAssociation(el);
}

GCoord GRHarmony::getLeftSpace() const
{
	GRSystemStartEndStruct * sse = mStartEndList.GetHead();
	if (!sse) return 0;
	GRTextSaveStruct * st = (GRTextSaveStruct *)sse->p;
	if (!st) return 0;

	return -st->boundingBox.left;
}

GCoord GRHarmony::getRightSpace() const
{
	GRSystemStartEndStruct * sse = mStartEndList.GetHead();
	if (!sse) return 0;
	GRTextSaveStruct * st = (GRTextSaveStruct *)sse->p;
	if (!st) return 0;

	return st->boundingBox.right;
}
