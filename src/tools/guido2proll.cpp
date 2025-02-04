#ifndef WIN32
#include <libgen.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#else
#pragma warning(disable:4996)
#endif

#include <string.h>

#include "GUIDOParse.h"
#include "GUIDOEngine.h"
#include "GUIDOPianoRoll.h"
#include "GUIDOScoreMap.h"
#include "Colors.h"
#include "SVGDevice.h"
#include "SVGSystem.h"

using namespace std;

const int  kDefaultWidth           = 1024;
const int  kDefaultHeight          = 512;
const int  kDefaultMinPitch        = -1;
const int  kDefaultMaxPitch        = -1;
//const bool kDefaultKeyboard        = false;
//const bool kDefaultVoicesAutoColor = false;
//const bool kDefaultMeasureBars     = false;
const int  kDefaultPitchLines      = kAutoLines;

const PianoRollType kDefaultPianoRoll = kSimplePianoRoll;

const char* kOptions[] = { "-help", "-pianoroll", "-width", "-height", "-start", "-end", "-minpitch", "-maxpitch", "-keyboard", "-voicesautocolor", "-measurebars", "-pitchlines" };
enum { kHelp, kPianoRoll, kWidth, kHeight, kStart, kEnd, kMinPitch, kMaxPitch, kKeyboard, kVoicesAutoColor, kMeasureBars, kPitchLines, kMaxOpt };

static void usage(char* name)
{
#ifndef WIN32
	const char* tool = basename (name);
#else
	const char* tool = name;
#endif
	cerr << "usage: " << tool << " [options] gmnfile" << endl;
	cerr << "options: -pianoroll       string : set the pianoroll type (default is " << kDefaultPianoRoll << ")" << endl;
	cerr << "                                        simple" << endl;
    cerr << "                                        trajectory" << endl;
	cerr << "         -width           value  : set the output width (default is " << kDefaultWidth << " -> width is adjusted to 1024)" << endl;   // REM: get default value from PianoRoll
	cerr << "         -height          value  : set the output height (default is " << kDefaultHeight << " -> height is adjusted to 512)" << endl; // REM: get default value from PianoRoll
	cerr << "         -start           date   : set time zone start (default is 0/0 -> start time is automatically adjusted)" << endl;
	cerr << "         -end             date   : set time zone end (default is 0/0 -> end time is automatically adjusted)" << endl;
    cerr << "         -minpitch        value  : set minimum midi pitch (default is " << kDefaultMinPitch << " -> min pitch is automatically adjusted)" << endl;
	cerr << "         -maxpitch        value  : set maximum midi pitch (default is " << kDefaultMaxPitch << " -> max pitch is automatically adjusted)" << endl;
	cerr << "         -keyboard               : enable keyboard" << endl;
	cerr << "         -voicesautocolor        : enable voices auto color" << endl;
	cerr << "         -measurebars            : enable measure bars" << endl;
	cerr << "         -pitchlines      string : set pitch lines display mode (default is auto)" << endl;
	cerr << "                                        auto" << endl;
    cerr << "                                        noline" << endl;

    exit(1);
}

static void error(GuidoErrCode err)
{
    if (err != guidoNoErr) {
        cerr << "error #" << err << ": " << GuidoGetErrorString (err) << endl;

        exit(err);
    }
}



//---------------------------------------------------------------------------------------------
static void checkusage(int argc, char **argv)
{
    if (argc == 1)
        usage(argv[0]);

    for (int i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], kOptions[kHelp]))
            usage(argv[0]);
        else if (*argv[i] == '-') {

            bool unknownOpt = true;
            for (int n = 1; (n < kMaxOpt) && unknownOpt; n++) {
                if (!strcmp (argv[i], kOptions[n]))
                    unknownOpt = false;
            }

            if (unknownOpt) usage(argv[0]);
		}
    }
}

static const char* getInputFile(int argc, char *argv[])
{
	const char * file = argv[argc-1];		// input file is the last arg
	if (*file == '-') usage(argv[0]);
	return file;
}

static GuidoDate ldateopt(int argc, char **argv, const char* opt, GuidoDate defaultvalue)
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp (argv[i], opt)) {
			i++;

			if (i >= argc)
                usage(argv[0]);
			else {
				int n,d;

				if (sscanf(argv[i], "%d/%d", &n, &d) == 2) {
					GuidoDate ret = {n, d};
					return ret;
				}
                else if (sscanf(argv[i], "%d", &n) == 1) {
					GuidoDate ret = {n, 1};
					return ret;
				}
                else
                    usage(argv[0]);
			}
		}
	}

	return defaultvalue;
}

static int lintopt(int argc, char **argv, const char* opt, int defaultvalue)
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], opt)) {
			i++;

			if (i >= argc)
                usage(argv[0]);
			else
				return atoi(argv[i]);
		}
	}

	return defaultvalue;
}

//---------------------------------------------------------------------------------------------
static bool lnoargopt(int argc, char **argv, const char* opt)
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], opt))
			return true;
	}
	return false;
}

static PianoRollType lPianoRollTypeopt(int argc, char **argv, const char* opt, PianoRollType defaultvalue)
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], opt)) {
			i++;

			if (i >= argc)
                usage(argv[0]);
			else {
                if (!strcmp(argv[i], "simple"))
                    return kSimplePianoRoll;
                else if (!strcmp(argv[i], "trajectory"))
                    return kTrajectoryPianoRoll;
                else
                    return kSimplePianoRoll;
            }
		}
	}

	return defaultvalue;
}

static int lPitchLinesopt(int argc, char **argv, const char* opt, int defaultvalue)
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], opt)) {
			i++;

			if (i >= argc)
                usage(argv[0]);
			else {
                if (!strcmp(argv[i], "auto"))
                    return kAutoLines;
                else if (!strcmp(argv[i], "noline"))
                    return kNoLine;
                /* Complete with other choices*/
            }
		}
	}

	return defaultvalue;
}

int main(int argc, char **argv)
{
 	SVGSystem sys;
//    SVGDevice dev(cout, &sys, 0);
	VGDevice *dev = sys.CreateDisplayDevice();
	
	checkusage(argc, argv);
	
	GuidoInitDesc gd = { dev, 0, 0, 0 };
    GuidoInit(&gd);

    const char* fileName = getInputFile(argc, argv);

	int  w               = lintopt       (argc, argv, kOptions[kWidth],           kDefaultWidth);
	int  h               = lintopt       (argc, argv, kOptions[kHeight],          kDefaultHeight);
    int  minPitch        = lintopt       (argc, argv, kOptions[kMinPitch],        kDefaultMinPitch);
    int  maxPitch        = lintopt       (argc, argv, kOptions[kMaxPitch],        kDefaultMaxPitch);

    bool keyboard        = lnoargopt (argc, argv, kOptions[kKeyboard]);
    bool voicesAutoColor = lnoargopt (argc, argv, kOptions[kVoicesAutoColor]);
    bool measureBars     = lnoargopt (argc, argv, kOptions[kMeasureBars]);
    int  pitchLines      = lPitchLinesopt(argc, argv, kOptions[kPitchLines],      kDefaultPitchLines);

    PianoRollType         pianoRollType = lPianoRollTypeopt(argc, argv, kOptions[kPianoRoll],  kDefaultPianoRoll);

	GuidoDate defDate = {0, 0};
	GuidoDate start   = ldateopt(argc, argv, kOptions[kStart], defDate);
	GuidoDate end     = ldateopt(argc, argv, kOptions[kEnd],   defDate);

	GuidoParser *parser = GuidoOpenParser();
	ARHandler    arh    = GuidoFile2AR(parser, fileName);

    GuidoErrCode err;
	if (arh) {
        PianoRoll *pianoRoll = GuidoAR2PianoRoll(pianoRollType, arh);
        
        LimitParams limitParams;
        limitParams.startDate = start;
        limitParams.endDate   = end;
        limitParams.lowPitch  = minPitch;
        limitParams.highPitch = maxPitch;

        /**** LIMITS ****/
        err = GuidoPianoRollSetLimits(pianoRoll, limitParams);
        error(err);
        /*********************/

        /**** KEYBOARD ****/
        err = GuidoPianoRollEnableKeyboard(pianoRoll, keyboard);
        error(err);

        float keyboardWidth;
        err = GuidoPianoRollGetKeyboardWidth(pianoRoll, h, keyboardWidth);
        error(err);
        /******************/

        /**** VOICES COLOR ****/
        err = GuidoPianoRollEnableAutoVoicesColoration(pianoRoll, voicesAutoColor);
        error(err);
		
        /**** MEASURE BARS ****/
        err = GuidoPianoRollEnableMeasureBars(pianoRoll, measureBars);
        error(err);
        /**********************/

        /**** PITCH LINES ****/
        err = GuidoPianoRollSetPitchLinesDisplayMode(pianoRoll, pitchLines);
        error(err);
        /*********************/

        /**** MAP ****/
        Time2GraphicMap map;
        err = GuidoPianoRollGetMap(pianoRoll, w, h, map);
        error(err);
        /*************/

        /**** DRAW ****/
		dev->NotifySize(w, h);
		dev->BeginDraw();
		dev->SelectPenColor(VGColor(100, 100, 100));
		dev->SelectFillColor(VGColor(0, 0, 0));
        err = GuidoPianoRollOnDraw(pianoRoll, w, h, dev);
 		dev->EndDraw();
		error(err);
        /**************/

        GuidoDestroyPianoRoll(pianoRoll);
		GuidoFreeAR(arh);
	}
	else {
		int line, col;
		
        err = GuidoParserGetErrorCode(parser, line, col, 0); // REM: l'erreur n'est pas r�cuper�e si l'arh a simplement mal �t� instanci�
		error(err);
	}

	GuidoCloseParser(parser);
    return 0;
}
