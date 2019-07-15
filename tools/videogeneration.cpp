#include <iostream>
#include <sstream>
#include <fstream>
#include <libmusicxml/libmusicxml.h>
#include "cairo_guido2img.h"
#include "engine.h"
#include "guidosession.h"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file audio_file output_video"
         << endl;
    return 1;
  }
  string musicxml_file = argv[1];
  string audio_file = argv[2];
  string output_video = argv[3];

  int page = 1;

  guidohttpd::makeApplication(argc, argv);
  guidohttpd::startEngine();

  GuidoLayoutSettings layoutSettings;
  layoutSettings.systemsDistance = 320;
  // layoutSettings.systemsDistance = 75;

  layoutSettings.systemsDistribution = kNeverDistrib;
  // layoutSettings.systemsDistribution = kAutoDistrib;
  layoutSettings.systemsDistribLimit = 0.25;
  layoutSettings.force = 750;
  layoutSettings.spring = 1.12;
  layoutSettings.neighborhoodSpacing = 0;
  layoutSettings.optimalPageFill = 1;
  layoutSettings.resizePage2Music = 1;
  layoutSettings.proportionalRenderingForceMultiplicator = 0;
  layoutSettings.checkLyricsCollisions = true;
  float youtube_ratio = 1280.0 / 720.0;
  float height = GuidoCM2Unit(29.7);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.layoutSettings = layoutSettings;
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.width = height * youtube_ratio;
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.height = height;

  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginleft = GuidoCM2Unit(1);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.margintop = GuidoCM2Unit(5);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginright = GuidoCM2Unit(1);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginbottom = GuidoCM2Unit(1);

  guidohttpd::guidosession::sDefaultScoreParameters.page = page;
  guidohttpd::guidosession::sDefaultScoreParameters.format = guidohttpd::GUIDO_WEB_API_PNG;
  // guidohttpd::guidosession::sDefaultScoreParameters.format = guidohttpd::GUIDO_WEB_API_SVG;

  std::stringstream guido;
  MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), false, guido);
  std::string svg_font_file = "/app/src/guido2.svg";
  guidohttpd::cairo_guido2img guido2img(svg_font_file);
  guidohttpd::guidosession* currentSession = new guidohttpd::guidosession(svg_font_file, guido.str(), "1shauishauis.gmm");
  // DA
  int pageCount = GuidoGetPageCount(currentSession->getGRHandler());
  std::cout << "PAGECOUNT:" << pageCount << std::endl;
  guidohttpd::GuidoSessionScoreParameters scoreParameters = guidohttpd::guidosession::sDefaultScoreParameters;

  int err = guido2img.convertScore(currentSession, scoreParameters);
  std::cout << "ERR:" << err << std::endl;
  if (err == 0) {
    ofstream myfile;
    myfile.open ("output.png");
    myfile.write(guido2img.data(), guido2img.size());
    myfile.close();
  }
  std::cout << guido2img.size() << std::endl;
  return 0;
}
