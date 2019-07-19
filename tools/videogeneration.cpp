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
#include "Fraction.h"

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


bool replace(std::string& str, const std::string& from, const std::string& to) {
  while (1) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
      return false;
    str.replace(start_pos, from.length(), to);
  }
  return true;
}


int parse_int(std::string& content, int& off) {
  int num = 0;
  while ((off < content.size()) && ((content[off] >= '0') && (content[off] <= '9'))) {
    num = num * 10 + (content[off] - '0');
    ++off;
  }
  return num;
}

float parse_float(std::string& content, int& off) {
  float num = 0;
  while ((off < content.size()) && ((content[off] >= '0') && (content[off] <= '9'))) {
    num = num * 10 + (content[off] - '0');
    ++off;
  }
  if (content[off] == '.') {
    ++off;
    int npow = 0;
    while ((off < content.size()) && ((content[off] >= '0') && (content[off] <= '9'))) {
      num = num * 10 + (content[off] - '0');
      ++off;
      ++npow;
    }
    num /= pow(10, npow);
  }
  return num;
}

bool parse_guido_date(std::string& content, int& off, Fraction& out) {
  int denom = 1;
  int num = parse_int(content, off);


  if (content[off] == '/') {
    ++off;
    denom = parse_int(content, off);
  }
  if (content[off] == ')') {
    ++off;
  }
  out.setNumerator(num);
  // out.setDenominator(denom * 4);
  out.setDenominator(denom);

}

bool erase_tag(std::string& guidostr, std::string tag) {
  size_t start_pos = guidostr.find("\\" + tag + "<\"");
  if (start_pos != string::npos) {
    size_t end_pos = guidostr.find(">", start_pos);
    std::cout << start_pos << " " << end_pos << std::endl;
    guidostr.erase(start_pos, 1 + end_pos - start_pos);
    return true;
  }
  return false;
}

bool parse_asco(std::string& asco_file, std::vector<std::pair<GuidoDate*, float> >& date_to_time) {
  std::cout << "parsing asco file:"
            << asco_file
            << std::endl;
  std::ifstream ifs(asco_file.c_str());
  std::string content;
  getline(ifs, content, '\0');
  // content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());
  replace(content, "\t", " ");
  replace(content, "\n", " ");
  replace(content, ",", " ");
  replace(content, "  ", " ");
  replace(content, "} }", "}}");

  size_t start_pos = content.find("NIM {") + 5;
  size_t end_pos = content.find("}}");

  content = content.substr(start_pos, end_pos - start_pos);
  // BEAT TIME BEAT TIME BEAT TIME
  replace(content, " ", "");
  int off = 0;
  Fraction cumul;

  while (off < content.size()) {
    if (content[off] == '(') {
      ++off;
    }
    Fraction out;
    parse_guido_date(content, off, out);
    std::cout << out.getNumerator() << " " << out.getDenominator() << std::endl;
    cumul += out;
    float time = parse_float(content, off);
    GuidoDate* date = new GuidoDate();
    date->num = cumul.getNumerator();
    // date->denom = cumul.getDenominator();
    date->denom = cumul.getDenominator();

    date_to_time.push_back(std::pair<GuidoDate*, float>(date, time));
    std::cout << "Time:" << time
              << " @" << cumul.getNumerator() << "/" << cumul.getDenominator()
              << std::endl;
  }
}


int main(int argc, char* argv[]) {
  if (argc < 3) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file asco_file"
         << endl;
    return 1;
  }
  string musicxml_file = argv[1];
  string asco_file = argv[2];
  std::vector<std::pair<GuidoDate*, float> > date_to_time;

  parse_asco(asco_file, date_to_time);
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
  std::string guidostr = guido.str();
  erase_tag(guidostr, "title");
  erase_tag(guidostr, "composer");
  std::cout << guidostr << std::endl;
  std::string svg_font_file = "/app/src/guido2.svg";
  guidohttpd::guidosession* currentSession = new guidohttpd::guidosession(svg_font_file, guidostr, "1shauishauis.gmm");
  currentSession->updateGRH(guidohttpd::guidosession::sDefaultScoreParameters);
  int pageCount = GuidoGetPageCount(currentSession->getGRHandler());
  std::cout << "PAGECOUNT:" << pageCount << std::endl;
  guidohttpd::GuidoSessionScoreParameters scoreParameters = guidohttpd::guidosession::sDefaultScoreParameters;
  png_stream_t fBuffer;
  fBuffer.data_ = new char[10485760];
  fBuffer.start_ = fBuffer.data_;
  fBuffer.size_ = 0;

  CGRHandler gr = currentSession->getGRHandler();
  TimeSegment t;
  FloatRect r;
  int err = 0;
  int nframe = 0;
  int current_page = 0;
  Time2GraphicMap systemMap;
  bool result = false;
  GuidoOnDrawDesc* main_desc;
  GuidoOnDrawDesc* desc = get_on_draw_desc(currentSession, scoreParameters);;
  CairoDevice* main_device;

  float fps = 24;
  float last_draw = 0;
  for (auto it = date_to_time.begin(); it != date_to_time.end(); ++it) {
    bool pageChange = (current_page == 0);

    std::cout << "Query beat " << *it->first << " @page " << current_page << std::endl;
    if (!pageChange) {
      result = GuidoGetTime(*it->first, systemMap, t, r);
      // SHOULD BE BEAT 60 at the end of page 1 !(measure 41)
      if (!result) {
        pageChange = true;
        // return 1;
      }
    }
    if (pageChange) {
      ++current_page;
      if (current_page > pageCount) {
        std::cout << "Aborted, no more page @"
                  << it->second << "s"
                  << std::endl;
        break;
      }
      main_desc = get_on_draw_desc(currentSession, scoreParameters);
      main_device = (CairoDevice*)main_desc->hdc;

      std::cout << "CURRENT PAGE:" << current_page << std::endl;
      main_desc->page = desc->page = current_page;
      err = convert_score_to_png(main_desc, fBuffer);
      if (err != 0) {
        std::cerr << "An error occured" << std::endl;
        return 1;
      }
      err = GuidoGetSystemMap(gr, current_page, width, height, systemMap);
      for (auto it = systemMap.begin(); it != systemMap.end(); it++) {
        std::cout << it->first.first << std::endl;
      }
      if (err != 0) {
        std::cerr << "An error occured" << std::endl;
        return 1;
      }
      result = GuidoGetTime(*it->first, systemMap, t, r);
      if (!result) {
        std::cerr << "Beat still not found after page change" << std::endl;
        return 1;
      }
    }
    err = convert_score_to_png(desc, fBuffer, &r, main_device, sizex, sizey);
    if (err == 0) {
      float duration = 2;
      auto next = it + 1;
      if (next != date_to_time.end()) {
        duration = next->second - it->second;
      }
      int target_frame = round((it->second + duration) * fps);
      int nframe_todraw = target_frame - nframe;
      for (int k = 0; k < nframe_todraw; ++k) {
        ofstream myfile;
        std::string output_file_path = "output" + std::to_string(nframe++) + ".png";
        myfile.open (output_file_path.c_str());
        myfile.write(fBuffer.start_, fBuffer.size_);
        myfile.close();
      }
      // break;
    }
    else {
      std::cerr << "An error occured" << std::endl;
      return 1;
    }
  }

  return 0;
  /*
    for (int current_page = 1; current_page <= pageCount; ++current_page) {
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
    std::cout << t.first << std::endl;
    // return 1;
    if (!result) {
    std::cerr << "Beat not found" << std::endl;
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
  */
  std::cout << "ALL DONE" << std::endl;
  return 0;
}
