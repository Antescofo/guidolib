/*
 * SetupDialog.h
 *
 * Created by Christophe Daudin on 17/04/08.
 * Copyright 2008 Grame. All rights reserved.
 *
 * This file may be used under the terms of the GNU General Public
 * License version 2.0 as published by the Free Software Foundation
 * and appearing in the file LICENSE.GPL included in the packaging of
 * this file.  Please review the following information to ensure GNU
 * General Public Licensing requirements will be met:
 * http://trolltech.com/products/qt/licenses/licensing/opensource/
 *
 * If you are unsure which license is appropriate for your use, please
 * review the following information:
 * http://trolltech.com/products/qt/licenses/licensing/licensingoverview
 * or contact the sales department at sales@trolltech.com.
 *
 * In addition, as a special exception, Trolltech gives you certain
 * additional rights. These rights are described in the Trolltech GPL
 * Exception version 1.0, which can be found at
 * http://www.trolltech.com/products/qt/gplexception/ and in the file
 * GPL_EXCEPTION.txt in this package.
 *
 * In addition, as a special exception, Trolltech, as the sole copyright
 * holder for Qt Designer, grants users of the Qt/Eclipse Integration
 * plug-in the right for the Qt/Eclipse Integration to link to
 * functionality provided by Qt Designer and its related libraries.
 *
 * Trolltech reserves all rights not expressly granted herein.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <QWidget>
#include <QSettings>
#include <QColorDialog>
#include <QFont>
#include <QtDebug>

#include "MainWindow.h"
#include "SetupDialog.h"
#include "GuidoHighlighter.h"

#define SETUP_DIALOG_POS_SETTING	"SetupDialogPosSetting"
#define SETUP_DIALOG_SIZE_SETTING	"SetupDialogSizeSetting"

#define SYNTAX_ELT_ID		"syntaxElementId"
#define BUTTON_COLOR		"buttonColor"
#define DIALOG_ELEMENT_ID	"dialogEltId"

QString weightToString( QFont::Weight w )
{
	switch (w)
	{
//		case QFont::Light:
//			return "Light";
//		break;
		case QFont::Normal:
			return "Normal";
		break;
//		case QFont::DemiBold:
//			return "DemiBold";
//		break;
		case QFont::Bold:
			return "Bold";
		break;
//		case QFont::Black:
//			return "Black";
//		break;
		default:
			return "";
	}
	return "";
}

QString weightToString(int weight) { return weightToString( QFont::Weight(weight) ); }

//-------------------------------------------------------------------------
SetupDialog::SetupDialog(MainWindow *parent) 
 : QDialog(parent), mMainWindow(parent)
{
	setupUi(this);

	QObject::connect (fDefaultButton, SIGNAL(clicked()), this, SLOT(reset()));
	QObject::connect (fSysDistBox, SIGNAL(valueChanged(int)), this, SLOT(setup()));
	QObject::connect (fMaxDistBox, SIGNAL(valueChanged(int)), this, SLOT(setup()));
	QObject::connect (fSpringBox, SIGNAL(valueChanged(int)), this, SLOT(setup()));
	QObject::connect (fForceBox, SIGNAL(valueChanged(int)), this, SLOT(setup()));
	QObject::connect (fSysDistrMenu, SIGNAL(currentIndexChanged(int)), this, SLOT(setup()));
	QObject::connect (fOPFcheckBox, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fNSpacingcheckBox, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fColorButton, SIGNAL(clicked()) , this, SLOT(changeColor()));
	
	QObject::connect (fMapping, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fBoundingBoxes, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fPageBB, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fSystemBB, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fSystemSliceBB, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fStaffBB, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fMeasureBB, SIGNAL(clicked()), this, SLOT(setup()));
	QObject::connect (fEventBB, SIGNAL(clicked()), this, SLOT(setup()));

	QObject::connect (fVoiceNumEdit, SIGNAL(valueChanged(int)), this, SLOT(voiceStaffSetup(int)));
	QObject::connect (fStaffNumEdit, SIGNAL(valueChanged(int)), this, SLOT(voiceStaffSetup(int)));
	QObject::connect (fShowAllStaffsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(voiceStaffSetup(int)));
	QObject::connect (fShowAllVoicesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(voiceStaffSetup(int)));

	mSavedSettings = mMainWindow->getEngineSettings();
	mSavedBBMap = mMainWindow->getBBMap();
	mSavedShowMapping = mMainWindow->getShowMapping();
	mSavedShowBoxes = mMainWindow->getShowBoxes();
	mSavedVoiceNum = mMainWindow->getVoiceNum();
	mSavedStaffNum = mMainWindow->getStaffNum();
	scoreColorChanged( mMainWindow->getScoreColor() );
	set (mSavedSettings, mSavedBBMap, mSavedShowMapping , mSavedShowBoxes , mSavedVoiceNum , mSavedStaffNum);

	mFontColorMap[ GuidoHighlighter::VOICE_SEPARATOR_ELT ]	= fVoiceSeparatorColorButton;
	mFontColorMap[ GuidoHighlighter::SCORE_SEPARATOR_ELT ]	= fScoreSeparatorColorButton;
	mFontColorMap[ GuidoHighlighter::DURATION_ELT ]			= fDurationsColorButton;
	mFontColorMap[ GuidoHighlighter::NOTE_ELT ]				= fNotesColorButton;
	mFontColorMap[ GuidoHighlighter::TAG_PARAM_ELT ]		= fTagsParametersColorButton;
	mFontColorMap[ GuidoHighlighter::TAG_ELT ]				= fTagsColorButton;
	mFontColorMap[ GuidoHighlighter::COMMENT_ELT ]			= fCommentsColorButton;
						
	mFontWeightMap[ GuidoHighlighter::VOICE_SEPARATOR_ELT ]	= fVoiceSeparatorWeight;
	mFontWeightMap[ GuidoHighlighter::SCORE_SEPARATOR_ELT ]	= fScoreSeparatorWeight;
	mFontWeightMap[ GuidoHighlighter::DURATION_ELT ]		= fDurationsWeight;
	mFontWeightMap[ GuidoHighlighter::NOTE_ELT ]			= fNotesWeight;
	mFontWeightMap[ GuidoHighlighter::TAG_PARAM_ELT ]		= fTagsParametersWeight;
	mFontWeightMap[ GuidoHighlighter::TAG_ELT ]				= fTagsWeight;
	mFontWeightMap[ GuidoHighlighter::COMMENT_ELT ]			= fCommentsWeight;

	fTabWidget->setCurrentIndex(0);

	for ( int i = 0 ; i < GuidoHighlighter::SIZE ; i++ )
	{
		// Set widgets properties with syntax-element id
		mFontColorMap[ i ]->setProperty( SYNTAX_ELT_ID , i );
		mFontWeightMap[ i ]->setProperty( SYNTAX_ELT_ID , i );
		
		// Set the color of the font-color button
		QColor c = mMainWindow->getHighlighter()->color( i );
		QPixmap pixmap(30 , 30);
		pixmap.fill( c );
		mFontColorMap[ i ]->setIcon( QIcon(pixmap) );
		mFontColorMap[ i ]->setProperty( BUTTON_COLOR , c );
				
		// Adds font-weight items in the combobox
		for ( int w = 0 ; w <= QFont::Black ; w++ )
		{
			QString weightString = weightToString( w );
			if ( weightString.size() )
				mFontWeightMap[i]->addItem( weightString , w );
		}
		// Set the current combobox item
		QFont::Weight weight = (QFont::Weight)mMainWindow->getHighlighter()->weight( i );
		mFontWeightMap[i]->setCurrentIndex( mFontWeightMap[i]->findData( weight ) );
		
		// Connect the font widgets to their methods
		connect( mFontColorMap[i] , SIGNAL(clicked()) , this , SLOT(fontColorButtonClicked()) );
		connect( mFontWeightMap[i] , SIGNAL(currentIndexChanged(int)) , this , SLOT(fontWeightChanged(int)) );

		mSavedColors[i] = c;
		mSavedWeights[i]= int(weight);
	}
	
	QSettings settings;
    QPoint pos = settings.value(SETUP_DIALOG_POS_SETTING, QPoint(300, 300)).toPoint();
    QSize winSize = settings.value(SETUP_DIALOG_SIZE_SETTING, sizeHint() ).toSize();
    resize(winSize);
    move(pos);
}

//-------------------------------------------------------------------------
SetupDialog::~SetupDialog()
{
	QSettings settings;
	settings.setValue(SETUP_DIALOG_POS_SETTING, pos());
    settings.setValue(SETUP_DIALOG_SIZE_SETTING, size());
}

//-------------------------------------------------------------------------
void SetupDialog::setup()
{
	GuidoLayoutSettings gls;
	int bbmap,voiceNum,staffNum;
	bool showBoxes, showMapping;
	get (gls, bbmap , showMapping , showBoxes , voiceNum,staffNum);
	mMainWindow->setEngineSettings (gls, bbmap , showMapping , showBoxes , voiceNum , staffNum);	
}

//-------------------------------------------------------------------------
void SetupDialog::changeColor()
{
	QColorDialog * colorDialog = new QColorDialog( mSavedColor , this );
	colorDialog->setWindowTitle("Choose score color");
	colorDialog->setOptions(QColorDialog::NoButtons);
	
	connect( colorDialog , SIGNAL( currentColorChanged(const QColor&)) , this , SLOT( scoreColorChanged(const QColor&) ) );
		
	colorDialog->open();
//	scoreColorChanged( colorDialog->selectedColor() );
}

//-------------------------------------------------------------------------
void SetupDialog::scoreColorChanged(const QColor& c)
{
	if ( c.isValid() )
	{
		mMainWindow->setScoreColor( c );
		QPixmap pixmap(30 , 30);
		pixmap.fill( c );
		fColorButton->setIcon( QIcon(pixmap) );
	}
}

//-------------------------------------------------------------------------
void SetupDialog::reject()
{
	mMainWindow->setEngineSettings (mSavedSettings, mSavedBBMap, mSavedShowMapping , mSavedShowBoxes , mSavedVoiceNum , mSavedStaffNum);
	scoreColorChanged(mSavedColor);

	GuidoHighlighter * highlighter = new GuidoHighlighter();
	for ( int i = 0 ; i < GuidoHighlighter::SIZE ; i++ )
		highlighter->addRule( i , mSavedColors[i] , QFont::Weight(mSavedWeights[i]) );
	mMainWindow->setHighlighter(highlighter);

	QDialog::reject();
}
	
//-------------------------------------------------------------------------
void SetupDialog::get (GuidoLayoutSettings& gls, int& bbmap, bool& showMapping, bool& showBoxes, int&voiceNum, int&staffNum)
{
	gls.systemsDistance		= fSysDistBox->value();
	gls.systemsDistribLimit = float(fMaxDistBox->value()) / 100;
	gls.spring				= float(fSpringBox->value()) / 100;
	gls.force				= fForceBox->value();
	gls.systemsDistribution	= fSysDistrMenu->currentIndex() + 1 ;
	gls.optimalPageFill		= fOPFcheckBox->checkState() == Qt::Checked ? 1 : 0;
	gls.neighborhoodSpacing	= fNSpacingcheckBox->checkState() == Qt::Checked ? 1 : 0;
	
	bbmap = kNoBB;
	if (fPageBB->checkState() == Qt::Checked)			bbmap |= kPageBB;
	if (fSystemBB->checkState() == Qt::Checked)			bbmap |= kSystemsBB;
	if (fSystemSliceBB->checkState() == Qt::Checked)	bbmap |= kSystemsSliceBB;
	if (fStaffBB->checkState() == Qt::Checked)			bbmap |= kStavesBB;
	if (fMeasureBB->checkState() == Qt::Checked)		bbmap |= kMeasureBB;
	if (fEventBB->checkState() == Qt::Checked)			bbmap |= kEventsBB;
	showMapping = fMapping->checkState();
	showBoxes = fBoundingBoxes->checkState();
	voiceNum = this->voiceNum();
	staffNum = this->staffNum();
}
	
//-------------------------------------------------------------------------
void SetupDialog::set (const GuidoLayoutSettings& gls, int bbmap , bool showMapping, bool showBoxes, int voiceNum, int staffNum)
{
	fSysDistBox->setValue(gls.systemsDistance);
	fMaxDistBox->setValue(gls.systemsDistribLimit*100);
	fSpringBox->setValue(gls.spring*100);
	fForceBox->setValue(gls.force);
	fSysDistrMenu->setCurrentIndex(gls.systemsDistribution - 1);
	fOPFcheckBox->setCheckState(gls.optimalPageFill ? Qt::Checked : Qt::Unchecked);
	fNSpacingcheckBox->setCheckState(gls.neighborhoodSpacing ? Qt::Checked : Qt::Unchecked);

	fMapping->setCheckState(showMapping ? Qt::Checked : Qt::Unchecked);
	fBoundingBoxes->setCheckState(showBoxes ? Qt::Checked : Qt::Unchecked);

	fPageBB->setCheckState(bbmap & kPageBB ? Qt::Checked : Qt::Unchecked);
	fSystemBB->setCheckState(bbmap & kSystemsBB ? Qt::Checked : Qt::Unchecked);
	fSystemSliceBB->setCheckState(bbmap & kSystemsSliceBB ? Qt::Checked : Qt::Unchecked);
	fStaffBB->setCheckState(bbmap & kStavesBB ? Qt::Checked : Qt::Unchecked);
	fMeasureBB->setCheckState(bbmap & kMeasureBB ? Qt::Checked : Qt::Unchecked);
	fEventBB->setCheckState(bbmap & kEventsBB ? Qt::Checked : Qt::Unchecked);
	
	fShowAllVoicesCheckBox->setCheckState( voiceNum==ALL_VOICE ? Qt::Checked : Qt::Unchecked );
	if ( (voiceNum!=ALL_VOICE) )
		fVoiceNumEdit->setValue(voiceNum);

	fShowAllStaffsCheckBox->setCheckState( staffNum==ALL_STAFF ? Qt::Checked : Qt::Unchecked );
	if ( (staffNum!=ALL_STAFF) )
		fStaffNumEdit->setValue(staffNum);
}

//-------------------------------------------------------------------------
int SetupDialog::voiceNum() const
{
	return ( fShowAllVoicesCheckBox->checkState() ) ? ALL_VOICE : fVoiceNumEdit->value();
}

//-------------------------------------------------------------------------
int SetupDialog::staffNum() const
{
	return ( fShowAllStaffsCheckBox->checkState() ) ? ALL_STAFF : fStaffNumEdit->value();
}
	
//-------------------------------------------------------------------------
void SetupDialog::reset()
{
	GuidoLayoutSettings gls;
	GuidoGetDefaultLayoutSettings (&gls);
	set (gls, kNoBB , false , false , ALL_VOICE , ALL_STAFF );
	setup();
	scoreColorChanged( Qt::black );
	
	GuidoHighlighter * highlighter = new GuidoHighlighter();
	for ( int i = 0 ; i < GuidoHighlighter::SIZE ; i++ )
		highlighter->addRule( i , mMainWindow->mDefaultFontColors[i] , QFont::Weight(mMainWindow->mDefaultFontWeights[i]) );
	mMainWindow->setHighlighter(highlighter);
}

//-------------------------------------------------------------------------
void SetupDialog::fontColorButtonClicked()
{
	QPushButton * button = (QPushButton *)sender();

	QColorDialog * colorDialog = new QColorDialog( button->property( BUTTON_COLOR ).value<QColor>() , this );
	colorDialog->setWindowTitle("Choose font color");
	colorDialog->setOptions(QColorDialog::NoButtons);
	colorDialog->setProperty( DIALOG_ELEMENT_ID , button->property( SYNTAX_ELT_ID ).toInt() );
	connect( colorDialog , SIGNAL( currentColorChanged(const QColor&)) , this , SLOT( fontColorChanged(const QColor&) ) );
	
	colorDialog->open();
}

//-------------------------------------------------------------------------
void SetupDialog::fontColorChanged(const QColor& color)
{
	QColorDialog * dialog = (QColorDialog *)sender();
	int syntaxEltId = dialog->property( DIALOG_ELEMENT_ID ).toInt();

	if ( color.isValid() )
	{
		QPixmap pixmap(30 , 30);
		pixmap.fill( color );
		mFontColorMap[syntaxEltId]->setIcon( QIcon(pixmap) );
		mFontColorMap[syntaxEltId]->setProperty( BUTTON_COLOR , color );
		mMainWindow->setHighlighter( syntaxEltId , color , mFontWeightMap[syntaxEltId]->itemData( mFontWeightMap[syntaxEltId]->currentIndex() ).toInt() );
	}
}

//-------------------------------------------------------------------------
void SetupDialog::fontWeightChanged(int index)
{
	QComboBox * combobox = (QComboBox *)sender();
	int id = combobox->property( SYNTAX_ELT_ID ).toInt();
	
	int weight = combobox->itemData( index ).toInt();
	QColor color = mFontColorMap[id]->property( BUTTON_COLOR ).value<QColor>();
	
	mMainWindow->setHighlighter( id , color , weight );
}

//-------------------------------------------------------------------------
void SetupDialog::voiceStaffSetup(int)
{
	fStaffNumEdit->setEnabled( !fShowAllStaffsCheckBox->checkState() );
	fVoiceNumEdit->setEnabled( !fShowAllVoicesCheckBox->checkState() );

	setup();
}
