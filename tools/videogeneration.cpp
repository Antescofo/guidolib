#include <iostream>
#include <sstream>
#include <fstream>
#include <libmusicxml/libmusicxml.h>
#include "cairo_guido2img.h"
#include "string.h"
#include "CairoSystem.h"
#include "CairoDevice.h"
#include "SVGSystem.h"
#include "SVGDevice.h"
#include "BinarySystem.h"
#include "BinaryDevice.h"

#include <cairo.h>
#include <Magick++.h>

#include <assert.h>

#include "engine.h"
#include "guidosession.h"

using namespace std;
using namespace guidohttpd;

typedef struct
{
  char *data_;
  char *start_;
  int size_;
  void reset() {
    size_ = 0;
    data_ = start_;
  }
} png_stream_t;

static cairo_status_t
write_png_stream_to_byte_array (void *in_closure, const unsigned char *data,
                                unsigned int length)
{
  png_stream_t* closure = (png_stream_t*)in_closure;

  memcpy(closure->data_, data, length);
  closure->data_ += length;
  closure->size_ += length;
  return CAIRO_STATUS_SUCCESS;
}

GuidoOnDrawDesc* get_on_draw_desc(guidosession* const currentSession, GuidoSessionScoreParameters &scoreParameters) {
  GuidoPageFormat curFormat;
  GuidoGetPageFormat(currentSession->getGRHandler(), scoreParameters.page, &curFormat);
  //int width = curFormat.width / 2.0;
  // int height = curFormat.height / 2.0;
  int width = 1280;
  int height = 720;
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create(surface);

  CairoSystem* sys = new CairoSystem(cr);

  // VGDevice * dev = sys->CreateDisplayDevice();
  VGDevice * dev = sys->CreateMemoryDevice(width, height);
  dev->SelectFillColor(VGColor(255,255,255));
  dev->Rectangle (0, 0, width, height);
  dev->SelectFillColor(VGColor(0,0,0));
  GuidoOnDrawDesc* desc = new GuidoOnDrawDesc();

  desc->handle = currentSession->getGRHandler();
  desc->hdc = dev;
  desc->page = scoreParameters.page;
  desc->updateRegion.erase = true;
  desc->scrollx = desc->scrolly = 0;
  desc->sizex = width;
  desc->sizey = height;
  desc->isprint = false;
  return desc;
}

//int convert_score_to_png(guidosession* const currentSession, GuidoSessionScoreParameters &scoreParameters, png_stream_t& fBuffer)
int convert_score_to_png(GuidoOnDrawDesc* desc, png_stream_t& fBuffer, FloatRect* r = 0, CairoDevice* other_device = 0, float sizex = 1, float sizey = 1)
{
  GuidoErrCode err = guidoNoErr;
  CairoDevice* cairo_device = (CairoDevice*)desc->hdc;
  fBuffer.reset();
  if (r && other_device) {
    cairo_device->CopyPixels(other_device);
    cairo_device->SelectPenColor(VGColor(0, 0, 255, 100));
    cairo_device->SelectFillColor(VGColor(0, 0, 255, 100));
    cairo_device->Rectangle(r->left * sizex, r->top * sizey, (r->left + (desc->sizex * 0.02)) * sizex, r->bottom * sizey);
    cairo_device->SelectFillColor(VGColor(0, 0, 0));
    cairo_device->SelectPenColor(VGColor(0, 0, 0));

  }
  else {
    cairo_device->SelectFillColor(VGColor(0, 0, 0));
    err = GuidoOnDraw(desc);
  }
  cairo_surface_t* surface = cairo_device->getSurface();
  cairo_surface_write_to_png_stream(surface, write_png_stream_to_byte_array, &fBuffer);
  return err;
}

struct Element {
  FloatRect box;
  TimeSegment dates;
  GuidoElementInfos infos;

  Element(const FloatRect& box_, const TimeSegment& dates_, const GuidoElementInfos& infos_) :
    box(box_),
    dates(dates_),
    infos(infos_)
    {
    }
};

class MyMapCollector : public MapCollector, public std::vector<Element> {
public:
  virtual void Graph2TimeMap( const FloatRect& box, const TimeSegment& dates, const GuidoElementInfos& infos ) {
    if (infos.type == kNote) {
      this->push_back(Element(box, dates, infos));
    }
  }
};


std::string to_string(int n) {
  std::stringstream ss;
  ss << n;
  return ss.str();
}


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
  /*
  layoutSettings.systemsDistance = 75;
  layoutSettings.systemsDistribution = kAutoDistrib;
  layoutSettings.systemsDistribLimit = 0.25;
  layoutSettings.force = 750;
  layoutSettings.spring = 1.1;
  layoutSettings.neighborhoodSpacing = 0;
  layoutSettings.optimalPageFill = 1;
  layoutSettings.resizePage2Music = 1;
  layoutSettings.proportionalRenderingForceMultiplicator = 0;
  layoutSettings.checkLyricsCollisions = false;
  */

  layoutSettings.systemsDistance = 320;
  layoutSettings.systemsDistribution = kAutoDistrib;
  layoutSettings.systemsDistribLimit = 0.25;
  layoutSettings.force = 750;
  layoutSettings.spring = 1.11;
  layoutSettings.neighborhoodSpacing = 0;
  layoutSettings.optimalPageFill = 1;
  layoutSettings.resizePage2Music = 0;
  layoutSettings.proportionalRenderingForceMultiplicator = 0;
  layoutSettings.checkLyricsCollisions = true;

  float youtube_ratio = 1280.0 / 720.0;
  float height = GuidoCM2Unit(25);
  float width = height * youtube_ratio;

  float sizex = 1280.0 / width;
  float sizey = 720.0 / height;

  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.layoutSettings = layoutSettings;
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.width = width;
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.height = height;
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginleft = GuidoCM2Unit(2);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.margintop = GuidoCM2Unit(2);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginright = GuidoCM2Unit(2);
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.marginbottom = GuidoCM2Unit(2);
  guidohttpd::guidosession::sDefaultScoreParameters.page = page;
  guidohttpd::guidosession::sDefaultScoreParameters.format = guidohttpd::GUIDO_WEB_API_PNG;
  // guidohttpd::guidosession::sDefaultScoreParameters.format = guidohttpd::GUIDO_WEB_API_SVG;

  std::stringstream guido;
  MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), false, guido);
  std::string svg_font_file = "/app/src/guido2.svg";
  guidohttpd::guidosession* currentSession = new guidohttpd::guidosession(svg_font_file, guido.str(), "1shauishauis.gmm");
  currentSession->updateGRH(guidohttpd::guidosession::sDefaultScoreParameters);
  int pageCount = GuidoGetPageCount(currentSession->getGRHandler());
  std::cout << "PAGECOUNT:" << pageCount << std::endl;
  guidohttpd::GuidoSessionScoreParameters scoreParameters = guidohttpd::guidosession::sDefaultScoreParameters;
  png_stream_t fBuffer;
  fBuffer.data_ = new char[10485760];
  fBuffer.start_ = fBuffer.data_;
  fBuffer.size_ = 0;

  CGRHandler gr = currentSession->getGRHandler();
  std::vector<std::pair<float, GuidoDate> > audio_to_beat_mapping;
  int current_page = 1;
  TimeSegment t;
  FloatRect r;
  int err = 0;
  int nframe = 0;

  for (int current_page = 1; current_page <= pageCount; ++current_page) {
    Time2GraphicMap beat_mapping;

    std::cout << "CURRENT PAGE:" << current_page << std::endl;
    MyMapCollector map_collector;
    GuidoGetMap(gr, current_page, width, height, kGuidoEvent, map_collector);
     //GuidoGetMap(gr, current_page, width, height, kGuidoSystem, map_collector);

    GuidoOnDrawDesc* main_desc = get_on_draw_desc(currentSession, scoreParameters);
    GuidoOnDrawDesc* desc = get_on_draw_desc(currentSession, scoreParameters);
    Time2GraphicMap systemMap;
    GuidoGetSystemMap(gr, current_page, width, height, systemMap);
    main_desc->page = desc->page = current_page;
    err = convert_score_to_png(main_desc, fBuffer);
    CairoDevice* main_device = (CairoDevice*)main_desc->hdc;
    if (err != 0) {
      std::cerr << "An error occured" << std::endl;
      return 1;
    }
    for (std::vector<Element>::iterator it = map_collector.begin(); it != map_collector.end(); it++) {
      FloatRect r;
      TimeSegment t;
      bool result = GuidoGetTime(it->dates.first, systemMap, t, r);
      if (!result) {
        std::cerr << "Beat not found" << std::endl;
        continue;
        return 1;
      }
      //FloatRect& r = it->box;
      err = convert_score_to_png(desc, fBuffer, &r, main_device, sizex, sizey);
      if (err == 0) {
        ofstream myfile;
        std::string output_file_path = "output" + std::to_string(nframe++) + ".png";
        myfile.open (output_file_path.c_str());
        myfile.write(fBuffer.start_, fBuffer.size_);
        myfile.close();
        std::cout << "Output image in " << output_file_path << std::endl;
        // break;
      }
      else {
        std::cerr << "An error occured" << std::endl;
        return 1;
      }
    }
  }
  std::cout << "ALL DONE" << std::endl;
  return 0;
}
