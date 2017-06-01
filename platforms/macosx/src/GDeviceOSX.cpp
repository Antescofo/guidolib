/*
  GUIDO Library
	Copyright (C) 2003--2006  Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

/////////////////////////////////////////////////////////////////
///
/// 	MacOS X Quartz 2D implementation of VGDevice.
///
///	perfs (G3-350): Bach-Inv1.gmn: parse: 240ms, draw: 670ms
/////////////////////////////////////////////////////////////////
  
#include <iostream>
#include <assert.h>
using namespace std;

#include "GDeviceOSX.h"
#include "GFontOSX.h"
#include "GSystemOSX.h" // guido hack - must be removed asap

/** Contains all the data necessary to identify a font */
typedef struct {
    const char *name;
    int size;
    int attributes;
} FontDescriptor;

/** Used by CFDictionary: prints the description of a font descriptor */
CFStringRef FontDescriptorCopyDescription(const void *value) {
    const FontDescriptor *descriptor = (const FontDescriptor *)value;
    return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s %d %d"), descriptor->name, descriptor->size, descriptor->attributes);
}

/** Used by CFDictionary: checks if two font descriptors are equal */
Boolean FontDescriptorEqual(const void *value1, const void *value2) {
    const FontDescriptor *descriptor1 = (const FontDescriptor *)value1;
    const FontDescriptor *descriptor2 = (const FontDescriptor *)value2;
    
    /* Size */
    if (descriptor1->size != descriptor2->size) {
        return false;
    }
    
    /* Attributes */
    if (descriptor1->attributes != descriptor2->attributes) {
        return false;
    }
    
    /* Name last, because it is the longest to check */
    if (strcmp(descriptor1->name, descriptor2->name) != 0) {
        return false;
    }
    
    return true;
}

/** Builds a hash for a C string, copied from  StackOverFlow */
CFHashCode FontDescriptorHashString(const char *str)
{
    CFHashCode hash = 5381;
    int c;
    
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    
    return hash;
}

/** Used by CFDictionary: builds a hash for a font descriptor */
CFHashCode FontDescriptorHash(const void *value) {
    const FontDescriptor *descriptor = (const FontDescriptor *)value;
    return FontDescriptorHashString(descriptor->name) ^ descriptor->size ^ (descriptor->attributes << 16);
}

/** Used by CFDictionary: releases a font descriptor. We should use a retain count but we don't, and we assume it is always 1. */
void FontDescriptorRelease(CFAllocatorRef allocator, const void *value) {
    const FontDescriptor *descriptor = (const FontDescriptor *)value;
    CFAllocatorDeallocate(allocator, (void*)descriptor->name);
    CFAllocatorDeallocate(allocator, (void*)value);
}

/** Used by CFDictionary: reatins a font descriptor. We should use a retain count but we don't, we assume it is in the stack and we copy
 it to the heap. */
const void *FontDescriptorRetain(CFAllocatorRef allocator, const void *value) {
    /* We allocate the value somewhere */
    const FontDescriptor *descriptor = (const FontDescriptor *)value;
    FontDescriptor *newDescriptor = (FontDescriptor*) CFAllocatorAllocate(allocator, sizeof(FontDescriptor), 0);
    char *newName = (char*)CFAllocatorAllocate(allocator, strlen(descriptor->name) + 1, 0);
    strcpy(newName, descriptor->name);
    newDescriptor -> name = newName;
    newDescriptor -> size = descriptor -> size;
    newDescriptor -> attributes = descriptor -> attributes;
    return newDescriptor;
}

/** Cache of fonts, shared between all the devices. */
static CFMutableDictionaryRef fontCache = NULL;


// --------------------------------------------------------------
GDeviceOSX::GDeviceOSX(int inWidth, int inHeight, VGSystem* sys)
{
	mPhysicalWidth = inWidth;
	mPhysicalHeight = inHeight;
	
	mCurrTextFont = NULL;
    mCurrTextFontAttribute = NULL;
	mCurrMusicFont = NULL;
	mRasterMode = kOpCopy;
//	mSymbolMap = NULL;
	mScaleX = mScaleY = 1;
	mOriginX = mOriginY = 0;

	
	mDPITag = 72.0f;

	// - Initial state of GDevice
	SetFontColor( VGColor(0,0,0,ALPHA_OPAQUE) );
	SetFontBackgroundColor( VGColor(255,255,255,ALPHA_TRANSPARENT) );	// transparent
	SetFontAlign( kAlignLeft | kAlignBase );
	MoveTo(0,0);

	// - GDeviceOSX specific ------------------
	mColorSpace = ::CGColorSpaceCreateDeviceRGB();
	
	// guido hack - must be removed asap
	mSys = sys;

    CFDictionaryKeyCallBacks keyCallBacks;
    keyCallBacks.copyDescription = FontDescriptorCopyDescription;
    keyCallBacks.equal = FontDescriptorEqual;
    keyCallBacks.hash = FontDescriptorHash;
    keyCallBacks.release = FontDescriptorRelease;
    keyCallBacks.retain = FontDescriptorRetain;
    if (! fontCache) {
        fontCache = CFDictionaryCreateMutable(kCFAllocatorDefault, 10, & keyCallBacks, & kCFTypeDictionaryValueCallBacks);
    }
}

// --------------------------------------------------------------
// process Quartz Init
void GDeviceOSX::Init()
{
	::CGContextSetFillColorSpace(mContext, mColorSpace);
	::CGContextSetStrokeColorSpace(mContext, mColorSpace);

	// - Rendering quality
	::CGContextSetInterpolationQuality(mContext, kCGInterpolationDefault); // kCGInterpolationHigh kCGInterpolationLow 
	::CGContextSetShouldAntialias(mContext, true);
	
	// Quartz device coordinate system has the (0,0) point at (bottom,left) when we expect it to be (top,left) 
	// so we setup an appropriate transformation matrix
#ifdef _USE_QD_COORDINATES_
	::CGContextScaleCTM(mContext, 1, -1 );
	::CGContextTranslateCTM(mContext, 0, -GetHeight());
#endif

	// - Setup Text Servces
	CGAffineTransform m = { 1.f, 0.f, 0.f, -1.f, 0.f, 0.f };
	CGContextSetTextMatrix (mContext, m);
//	CGAffineTransform textCTM = ::CGContextGetTextMatrix(mContext);
//	::CGContextSetTextMatrix(mContext, ::CGAffineTransformScale( textCTM, 1, -1 ));
	::CGContextSetTextDrawingMode(mContext, kCGTextFill );	// kCGTextFillStroke
}

// --------------------------------------------------------------
GDeviceOSX::~GDeviceOSX()
{
	::CGColorSpaceRelease(mColorSpace);
    if (mCurrTextFontAttribute) {
        CFRelease(mCurrTextFontAttribute);
    }
}

/////////////////////////////////////////////////////////////////
// - Drawing services -------------------------------------------
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
bool GDeviceOSX::BeginDraw()
{
	// - save Quartz context -------------------
	::CGContextSaveGState(mContext);	

	// - save device's context - was SaveDC() --
	GState			s;
	s.pen			= mPen;
	s.fillColor		= mFillColor;
	s.textColor		= mTextColor;
	s.textBackColor = mTextBackColor;
	s.scaleX		= mScaleX;
	s.scaleY		= mScaleY;
	s.originX		= mOriginX;
	s.originY		= mOriginY;
	s.textAlign		= mTextAlign;
	s.currTextFont	= mCurrTextFont;
	s.currMusicFont = mCurrMusicFont;
	s.rasterMode	= mRasterMode;

	mStateStack.push( s );
	return true;
}

// --------------------------------------------------------------
void GDeviceOSX::EndDraw()
{
	// - restore Quartz context ------------------
	::CGContextRestoreGState(mContext); 

	// - restore device's state - was RestoreDC()
	GState & s = mStateStack.top();
	
	mPen			= s.pen;
	mFillColor		= s.fillColor;
	mTextColor		= s.textColor;	
	mTextBackColor	= s.textBackColor;	
	mScaleX			= s.scaleX;
	mScaleY			= s.scaleY;
	mOriginX		= s.originX;
	mOriginY		= s.originY;
	mTextAlign		= s.textAlign;
	mCurrTextFont	= s.currTextFont;
	mCurrMusicFont	= s.currMusicFont;
	mRasterMode		= s.rasterMode;
	
	mStateStack.pop();
}

// --------------------------------------------------------------
void GDeviceOSX::SetRasterOpMode( VRasterOpMode ROpMode)
{ 
	mRasterMode = ROpMode; 
	CGBlendMode mode;
	switch (mRasterMode) {
		case kOpCopy:		mode = kCGBlendModeNormal;
			break;
		case kOpAnd:		mode = kCGBlendModeMultiply;
			break;
		case kOpXOr:		mode = kCGBlendModeNormal;	// ‡ voir !!
			break;
		case kOpInvert:		mode = kCGBlendModeNormal;	// ‡ voir !!
			break;
		case kOpOr:			mode = kCGBlendModeNormal;	// ‡ voir !!
			break;
		default:
			mode = kCGBlendModeNormal;
	}
	::CGContextSetBlendMode(mContext, mode);
}

// --------------------------------------------------------------
void GDeviceOSX::MoveTo( float x, float y )
{
	mCurrPenPos.x = x;
	mCurrPenPos.y = y;
}

// --------------------------------------------------------------
void GDeviceOSX::LineTo( float x, float y )
{
	Line(mCurrPenPos.x, mCurrPenPos.y, x, y);
}

// --------------------------------------------------------------
void GDeviceOSX::Line( float x1, float y1, float x2, float y2 )
{
	::CGContextBeginPath(mContext);
	::CGContextMoveToPoint(mContext, x1, y1); 
	::CGContextAddLineToPoint(mContext, x2, y2);
	::CGContextStrokePath(mContext);

//	mCurrPenPos.x = x2;
//	mCurrPenPos.y = y2;
	MoveTo( x2,y2 );
}

// --------------------------------------------------------------
void GDeviceOSX::Frame( float left, float top, float right, float bottom )
{
	::CGContextBeginPath(mContext);
	::CGContextMoveToPoint(mContext, left, top); 
	::CGContextAddLineToPoint(mContext, left, bottom);
	::CGContextAddLineToPoint(mContext, right, bottom);
	::CGContextAddLineToPoint(mContext, right, top);
	::CGContextAddLineToPoint(mContext, left, top);
	::CGContextStrokePath(mContext);
	
	MoveTo( left,top );
}

// --------------------------------------------------------------
void GDeviceOSX::Arc( float left, float top, float right, float bottom, float startX, float startY, float endX, float endY )
{
	// - Save current state
	::CGContextSaveGState(mContext);
	
	// - Preprocess
	const float midX = (left + right) * 0.5f;
	const float midY = (top + bottom) * 0.5f;
	const float width = right - left;
	const float height = bottom - top;

	// - CGContextAddArc does not support ellipses, so we need
	// 	 to scale the device context first.
	const float yScale = height / width;
	::CGContextScaleCTM(mContext, 1, yScale);
	
	// - Draw
	const float angle1 = CoordToDegree( startX - midX, startY - midY );		
	const float angle2 = CoordToDegree( endX - midX, endY - midY );

	//::CGContextAddArc(mContext, midX, midY, width, angle1, angle2, 0);
	::CGContextAddArc(mContext, midX, midY, width, angle1, angle2, 0);
	::CGContextClosePath(mContext);
	::CGContextStrokePath(mContext); 

	// - Restore State
	::CGContextRestoreGState(mContext);
}

// --------------------------------------------------------------
void GDeviceOSX::Triangle( float x1, float y1, float x2, float y2, float x3, float y3 )
{
	const float xCoords [] = { x1, x2, x3 };
	const float yCoords [] = { y1, y2, y3 };
	
	Polygon(xCoords, yCoords, 3); 
}

// --------------------------------------------------------------
void GDeviceOSX::Polygon( const float * xCoords, const float * yCoords, int count )
{
	::CGContextBeginPath(mContext );
	::CGContextMoveToPoint(mContext, xCoords[ count - 1 ], yCoords[ count - 1 ] ); 

	for( int index = 0; index < count; index ++ )
		::CGContextAddLineToPoint(mContext, xCoords[ index ], yCoords[ index ] );

	::CGContextClosePath(mContext);
//	::CGContextDrawPath(mContext, kCGPathFillStroke);
	::CGContextDrawPath(mContext, kCGPathFill);
}
// --------------------------------------------------------------
void GDeviceOSX::Rectangle( float left, float top, float right, float bottom )
{
	CGRect myRect = ::CGRectMake( left, top, right - left, bottom - top );

	::CGContextBeginPath(mContext);
	::CGContextAddRect(mContext, myRect );
//	::CGContextDrawPath(mContext, kCGPathFillStroke );
	::CGContextDrawPath(mContext, kCGPathFill );
}

/////////////////////////////////////////////////////////////////
// - Font services ----------------------------------------------
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
void GDeviceOSX::SetMusicFont( const VGFont * inObj )
{
	// CGContextSelectFont does not return an error code, so we first select
	// a well-known font. If we did not, musical symbols may be displayed instead of 
	// plain-text.

//	::CGContextSelectFont(mContext, "Helvetica", inObj->GetSize(), kCGEncodingMacRoman  );	// ok
	::CGContextSelectFont(mContext, inObj->GetName(), inObj->GetSize(), kCGEncodingFontSpecific );// ok
	
	mCurrMusicFont = inObj;
	
}
const char* convertGuidoFontNameToCoreText(const char *name, int properties) {
    
    if (strcmp(name, "Arial") == 0)
    {
        switch (properties) {
            case GFontOSX::kFontBold:
                return "Arial-BoldMT";
            case GFontOSX::kFontItalic:
                return "Arial-ItalicMT";
            default:
                return "ArialMT";
        }
    }else if (strcmp(name, "Palatino") == 0)
    {
        switch (properties) {
            case GFontOSX::kFontBold:
                return "Palatino-Bold";
            case GFontOSX::kFontItalic:
                return "Palatino-Italic";
            default:
                return "Palatino";
        }
    }else if (strcmp(name, "Baskerville") == 0)
    {
        switch (properties) {
            case GFontOSX::kFontBold:
                return "Baskerville-Bold";
            case GFontOSX::kFontItalic:
                return "Baskerville-Italic";
            default:
                return "Baskerville";
        }
    }else if (strcmp(name, "Didot") == 0)
    {
        switch (properties) {
            case GFontOSX::kFontBold:
                return "Didot-Bold";
            case GFontOSX::kFontItalic:
                return "Didot-Italic";
            default:
                return "Didot";
        }
    }else if (strcmp(name, "Times New Roman") == 0)
    {
        switch (properties) {
            case GFontOSX::kFontBold:
                return "TimesNewRomanPS-BoldMT";
            case GFontOSX::kFontItalic:
                return "TimesNewRomanPS-ItalicMT";
            default:
                return "TimesNewRomanPSMT";
        }
    }else {
        /* Default: Georgia */
        switch (properties) {
            case GFontOSX::kFontBold:
                return "Georgia-Bold";
            case GFontOSX::kFontItalic:
                return "Georgia-Italic";
            default:
                return "Georgia";
        }
    }
    
}

/* Creates a CTFont from a VGFont */
CTFontRef convertVGFontToCTFont(const VGFont *vgfont) {
    
    /* Both name and properties make up the name of the CTFont */
    const char *name = convertGuidoFontNameToCoreText(vgfont->GetName(), vgfont->GetProperties());
    
    /* Create the CTFont */
    return CTFontCreateWithName( CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8) , vgfont->GetSize(), NULL);
    
}

// --------------------------------------------------------------
void GDeviceOSX::SetTextFont( const VGFont* inObj )
{
    if (! inObj) {
        return;
    }

    /* Save the font */
    mCurrTextFont = inObj;
    
    /* Forget about the old attribute dictionary */
    if (mCurrTextFontAttribute) {
        CFRelease(mCurrTextFontAttribute);
    }
    
    /* Build a description of the font, to be used by the cache */
    FontDescriptor descriptor;
    descriptor.name = inObj -> GetName();
    descriptor.size = inObj -> GetSize();
    descriptor.attributes = inObj ->  GetProperties();
    
    /* Check if the font is cached */
    CFDictionaryRef cachedAttribute = (CFDictionaryRef) CFDictionaryGetValue(fontCache, & descriptor);
    if (cachedAttribute) {
        mCurrTextFontAttribute = cachedAttribute;
        CFRetain(mCurrTextFontAttribute);
        return;
    }
    
    /* The font is not cached, create it */
    CTFontRef ctfont = convertVGFontToCTFont(inObj);
    
    /* Create the attribute dictionary containing the font */
    const void *keys = kCTFontAttributeName;
    const void *values = ctfont;
    mCurrTextFontAttribute = CFDictionaryCreate(kCFAllocatorDefault, &keys, &values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFRelease(ctfont);
    
    /* Cache the attribute dictionary */
    CFDictionarySetValue(fontCache, (const void*)& descriptor, (const void*)mCurrTextFontAttribute);
}



/////////////////////////////////////////////////////////////////
// - Pen & brush services ---------------------------------------
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
void GDeviceOSX::SelectPen( const VGColor & c, float width )
{
	SelectPenColor(c);
	SelectPenWidth(width);
/*
	mPen.Set(c, width);	
	float color[4];
	MakeCGColor(c, color);
	::CGContextSetStrokeColor(mContext, color);
	::CGContextSetLineWidth(mContext, width);
*/
}

void GDeviceOSX::SelectPenWidth( float width )
{
	mPen.Set(width);	
	::CGContextSetLineWidth(mContext, width);
}

void GDeviceOSX::SelectPenColor( const VGColor & c)
{
	mPen.Set(c);	
	CGFloat color[4];
	MakeCGColor(c, color);
	::CGContextSetStrokeColor(mContext, color);
}

// --------------------------------------------------------------
void GDeviceOSX::SelectFillColor( const VGColor & c )
{
	mFillColor.Set(c);
	
	CGFloat color[4];
	MakeCGColor(c, color);
	::CGContextSetFillColor(mContext, color);
}

// --------------------------------------------------------------
// Save the current pen, select the new one
void GDeviceOSX::PushPen( const VGColor & inColor, float inWidth )
{
	mPenStack.push( mPen );
	SelectPen( inColor, inWidth );
}

// --------------------------------------------------------------
// Restore the previous pen from the stack
void GDeviceOSX::PopPen()
{
	VGPen & pen = mPenStack.top();
	SelectPen( pen.mColor, pen.mWidth );
	mPenStack.pop();
}

// --------------------------------------------------------------
void GDeviceOSX::PushPenColor( const VGColor & inColor)
{
	mPenColorStack.push(mPen.mColor);
	SelectPenColor (inColor);
}

// --------------------------------------------------------------
void GDeviceOSX::PopPenColor()
{
	SelectPenColor(mPenColorStack.top());
	mPenColorStack.pop();
}

// --------------------------------------------------------------
void GDeviceOSX::PushPenWidth( float width)
{
	mPenWidthStack.push(mPen.mWidth);
	SelectPenWidth (width);
}

// --------------------------------------------------------------
void GDeviceOSX::PopPenWidth()
{
	SelectPenWidth(mPenWidthStack.top());
	mPenWidthStack.pop();
}

// --------------------------------------------------------------
// Save the current brush, select the new one
void
GDeviceOSX::PushFillColor( const VGColor & inColor )
{
	mBrushStack.push( mFillColor );
	if( inColor != mFillColor )
		SelectFillColor( inColor );
}

// --------------------------------------------------------------
// Restore the previous brush from the stack
void
GDeviceOSX::PopFillColor()
{
	VGColor & brush = mBrushStack.top();
	SelectFillColor( brush );
	mBrushStack.pop();
}

/////////////////////////////////////////////////////////////////
// - Bitmap services (bit-block copy methods) -------------------
/////////////////////////////////////////////////////////////////

// Quartz Doc : After drawing in a bitmap context, you can easily 
// transfer the bitmap image to another Quartz context of any type. 
// To maintain device independence, copying an image is a drawing 
// operation and not a blitting operation. There are two steps:

// - Create a Quartz image from the bitmap. In Mac OS X v10.4 and later, 
// you can use the function CGBitmapContextCreateImage.
// - Draw the image in the destination context using the function 
// CGContextDrawImage.
// --------------------------------------------------------------
bool
GDeviceOSX::CopyPixels( VGDevice* pSrcDC, float alpha) 
{ 
	return CopyPixels( int(mOriginX), int(mOriginY), pSrcDC, 
					   int(pSrcDC->GetXOrigin()), int(pSrcDC->GetYOrigin()),
					   pSrcDC->GetWidth(), pSrcDC->GetHeight(), alpha );
}

// --------------------------------------------------------------
bool
GDeviceOSX::CopyPixels( int xDest, int yDest, VGDevice* pSrcDC, 
						int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, float alpha )	
{ 
	GDeviceOSX* pSrcDC_OSX = (GDeviceOSX*)(pSrcDC);
	CGContextRef srcContext = (CGContextRef)( pSrcDC_OSX->GetNativeContext() );
	
	yDest = mPhysicalHeight - yDest;
	
	CGImageRef srcPixmap = ::CGBitmapContextCreateImage( srcContext );
	CGRect srcRect = ::CGRectMake( xSrc, ySrc, nSrcWidth, nSrcHeight );
	CGImageRef srcRectPixmap = ::CGImageCreateWithImageInRect( srcPixmap, srcRect );
	CGRect destRect = ::CGRectMake( xDest, yDest, nSrcWidth, -nSrcHeight );
	
	// We need to setup the desination context in Quartz default coordinate system before doing the copy
#ifdef _USE_QD_COORDINATES_
	::CGContextScaleCTM(mContext, 1, -1 );
	::CGContextTranslateCTM(mContext, 0, -GetHeight());
#endif
	::CGContextSetAlpha(mContext, alpha); 
	
	::CGContextDrawImage(mContext, destRect, srcRectPixmap);
	
#ifdef _USE_QD_COORDINATES_
	::CGContextScaleCTM(mContext, 1, -1 );
	::CGContextTranslateCTM(mContext, 0, -GetHeight());
#endif
	::CGContextSetAlpha(mContext, 1.0); 
	
	::CGImageRelease(srcPixmap);
	::CGImageRelease(srcRectPixmap);
	return true; 
}

// --------------------------------------------------------------
bool
GDeviceOSX::CopyPixels( int xDest, int yDest, 
						int dstWidth, int dstHeight, 
						VGDevice* pSrcDC, float alpha ) 
{ 
	GDeviceOSX* pSrcDC_OSX = (GDeviceOSX*)(pSrcDC);
	
	yDest = mPhysicalHeight - yDest;

	CGContextRef srcContext = (CGContextRef)( pSrcDC_OSX->GetNativeContext() );
	CGImageRef srcPixmap = ::CGBitmapContextCreateImage( srcContext );
	CGRect destRect = ::CGRectMake( xDest, yDest, dstWidth, -dstHeight );
	
	// We need to setup the desination context in Quartz default coordinate system before doing the copy
#ifdef _USE_QD_COORDINATES_
	::CGContextScaleCTM(mContext, 1, -1 );
	::CGContextTranslateCTM(mContext, 0, -GetHeight());
#endif
	::CGContextSetAlpha(mContext, alpha); 
	
	::CGContextDrawImage (mContext, destRect, srcPixmap);
	
#ifdef _USE_QD_COORDINATES_
	::CGContextScaleCTM(mContext, 1, -1 );
	::CGContextTranslateCTM(mContext, 0, -GetHeight());
#endif
	::CGContextSetAlpha(mContext, 1.0); 
	
	::CGImageRelease(srcPixmap);
	return true; 
}

// --------------------------------------------------------------
bool
GDeviceOSX::CopyPixels( int xDest, int yDest, 
						int dstWidth, int dstHeight, 
						VGDevice* pSrcDC, 
						int xSrc, int ySrc, 
						int nSrcWidth, int nSrcHeight, float alpha) 
{ 
	return false; 
} 

/////////////////////////////////////////////////////////////////
// - Coordinate services ----------------------------------------
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
void			
GDeviceOSX::SetScale( float x, float y )
{
	mScaleX *= x;
	mScaleY *= y;
	
	// - GDeviceOSX specific ------------------
	::CGContextScaleCTM(mContext, (x ), (y ));
}

// --------------------------------------------------------------
// Called by: GRPage, GRStaff, GRSystem
void				
GDeviceOSX::SetOrigin( float x, float y )
{
	const float prevX = mOriginX;
	const float prevY = mOriginY;
	mOriginX = x;
	mOriginY = y;

	// - GDeviceOSX specific / was DoSetOrigin -----------------
	::CGContextTranslateCTM(mContext, (x - prevX), (y - prevY));
}

// --------------------------------------------------------------
// Called by: GRPage, GRStaff, GRSystem
void				
GDeviceOSX::OffsetOrigin( float x, float y )
{
	const float prevX = mOriginX;
	const float prevY = mOriginY;
	mOriginX += x;
	mOriginY += y;

	// - GDeviceOSX specific / was DoSetOrigin -----------------
	::CGContextTranslateCTM(mContext, (mOriginX - prevX), (mOriginY - prevY));
}

// --------------------------------------------------------------
// Called by:
// GRMusic::OnDraw
// GRPage::getPixelSize
void			
GDeviceOSX::LogicalToDevice( float * x, float * y ) const
{
	*x = (*x * mScaleX - mOriginX);
	*y = (*y * mScaleY - mOriginY);
}

// --------------------------------------------------------------
// Called by:
// GRPage::DPtoLPRect
// GRPage::getVirtualSize
void			
GDeviceOSX::DeviceToLogical( float * x, float * y ) const	
{
	*x = ( *x + mOriginX ) / mScaleX;
	*y = ( *y + mOriginY ) / mScaleY;
}

/////////////////////////////////////////////////////////////////
// - Text and music symbols services ----------------------------
/////////////////////////////////////////////////////////////////
// --------------------------------------------------------------
void GDeviceOSX::DrawMusicSymbol( float x, float y, unsigned int inSymbolID )
{
	GFontOSX * macFont = (GFontOSX *)mCurrMusicFont;
	assert(macFont);
	CGGlyph glyph = macFont->GetGlyph( inSymbolID );

	// - Calculate string dimensions		
	// - Perform text alignement. TODO: use precalculated values.
	if( mTextAlign != kAlignBaseLeft )
	{	
		float w = 0;
		float h = 0;
		float baseline = 0;
		mCurrMusicFont->GetExtent( (char)inSymbolID, &w, &h, this );
		if( mTextAlign & kAlignBottom )	// Vertical align
			y -= baseline; 
		else if( mTextAlign & kAlignTop )
			y += h - baseline;

		if( mTextAlign & kAlignRight )	// Horizontal align
			x -= w;
		else if( mTextAlign & kAlignCenter )
			x -= (w * float(0.5));
	}

	::CGContextSelectFont(mContext, mCurrMusicFont->GetName(), mCurrMusicFont->GetSize(), kCGEncodingMacRoman  );// ok

	// - Draw text
	PushFillColor( VGColor(mTextColor.mRed, mTextColor.mGreen, mTextColor.mBlue, mTextColor.mAlpha) );
	::CGContextShowGlyphsAtPoint(mContext, x, y, &glyph, 1 );
	PopFillColor();
}
// --------------------------------------------------------------

/* Does not support multiline, if we need it we have to replace the CTLineRef with a CTFramesetterRef.
 May not support OS X because the code assumes that the y-coordinate is flipped. */
void GDeviceOSX::DrawString( float x, float y, const char * s, int inCharCount )
{
    /* Empty check */
    if (s == NULL || inCharCount == 0) {
        return;
    }
    
    // Create an attributed string
    CFStringRef cfstring = CFStringCreateWithCString(kCFAllocatorDefault, s, kCFStringEncodingUTF8);
    CFAttributedStringRef attributedOverlayText = CFAttributedStringCreate(kCFAllocatorDefault, cfstring, mCurrTextFontAttribute);
    CFRelease(cfstring);
    
    /// Create a single line containing the string
    CTLineRef line = CTLineCreateWithAttributedString(attributedOverlayText);
    CFRelease(attributedOverlayText);
    
    /* Get the bounds of the text, as if we drew it in the context without changing x,y */
    CGContextSetTextPosition(mContext, x, y);
    CGRect bounds = CTLineGetImageBounds(line, mContext);
    
    /* Get the ascent and descent */
    CGFloat ascent, descent;
    CTLineGetTypographicBounds(line, & ascent, & descent, NULL);
    
    // - Perform text alignement. If we change nothing, the line is drawn (by CGContext) at baseline, left aligned
    if (mTextAlign & kAlignBottom) {
        y -= descent;
    }
    if (mTextAlign & kAlignTop) {
        y += ascent;
    }
    if (mTextAlign & kAlignCenter) {
        x -= bounds.size.width / 2;
    }
    if (mTextAlign & kAlignRight) {
        x -= bounds.size.width;
    }
    
    // Core Text changes the state of the context, so save it
    CGContextSaveGState(mContext);
    
    /* Move to the real position */
    CGContextSetTextPosition(mContext, x, y);
    
    /* Draw text background (we use the image bounds of the text but it would be prettier with the typographic bounds) */
    if (mTextBackColor.mAlpha) {
        PushPen( mTextBackColor, 1 );
        PushFillColor( mTextBackColor );
        CGRect finalBounds = CTLineGetImageBounds(line, mContext);
        CGContextFillRect(mContext, finalBounds);
        PopFillColor();
        PopPen();
    }
    
    /// Draw the line, set the color
    PushFillColor( VGColor(mTextColor.mRed, mTextColor.mGreen, mTextColor.mBlue,  mTextColor.mAlpha) );
    CTLineDraw(line, mContext);
    CFRelease(line);
    PopFillColor();

    // Restore the state of the contexte
    CGContextRestoreGState(mContext);
    
    
}

// --------------------------------------------------------------
void *			
GDeviceOSX::GetNativeContext() const
{
	return mContext; 
}

// --------------------------------------------------------------
void			
GDeviceOSX::MakeCGColor( const VGColor & inColor, CGFloat outColor[ 4 ] )
{
	const float conv = 1.0f / 255.0f;

	outColor[ 0 ] = float(inColor.mRed) * conv;
	outColor[ 1 ] = float(inColor.mGreen) * conv;
	outColor[ 2 ] = float(inColor.mBlue) * conv;
	outColor[ 3 ] = float(inColor.mAlpha) * conv;	
}

// --------------------------------------------------------------
// Return an angle from 0 to 360
float 
GDeviceOSX::CoordToDegree( float x, float y )
{
	const float kRadToDeg = 180 / 3.14159265359f;
	float outAngle = (float)atan2( x, y ) * kRadToDeg;
	if( outAngle < 0 )
		outAngle += 360;

	return outAngle;
}

