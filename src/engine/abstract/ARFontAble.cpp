/*
  GUIDO Library
  Copyright (C) 2017 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include "ARFontAble.h"
#include "TagParameterStrings.h"
#include "TagParameterString.h"
#include "TagParameterFloat.h"
#include "FontManager.h"

using namespace std;


//--------------------------------------------------------------------------
ARFontAble::ARFontAble()
{
	setupTagParameters (gMaps->sARFontAbleMap);
}

//--------------------------------------------------------------------------
void ARFontAble::setTagParameters (const TagParameterMap& params)
{
	fTextFormat = getParameter<TagParameterString>(kTextFormatStr, true)->getValue();
	fFontName	= getParameter<TagParameterString>(kFontStr, true)->getValue();
    if (fFontName.empty()) {
        fFontName = FontManager::kDefaultTextFont;
    }
	fFontSize	= getParameter<TagParameterFloat>(kFSizeStr, true)->getValue();
	fTextAttributes = getParameter<TagParameterString>(kFAttributesStr, true)->getValue();
}

