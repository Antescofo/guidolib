#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <map>
#include <libmusicxml/libmusicxml.h>
#include <fluidsynth.h>

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
  guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.margintop = GuidoCM2Unit(4);

  if (r && other_device) {
    cairo_device->CopyPixels(other_device);
    cairo_device->SelectPenColor(VGColor(0, 0, 255, 100));
    cairo_device->SelectFillColor(VGColor(0, 0, 255, 100));
    if (r->right < r->left) {
      std::swap(r->right, r->left);
    }

    cairo_device->Rectangle(r->left * sizex, r->top * sizey, (r->left + (desc->sizex * 0.04)) * sizex, r->bottom * sizey);
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
  GuidoDate date;
  // TimeSegment dates;
  GuidoElementInfos infos;
  int event_type; // 0 for rest, 1 for note on, 2 for note off
  int page;
  float time;

  Element(const FloatRect& box_, const GuidoDate& date_, const GuidoElementInfos& infos_, int page_, int event_type_) :
    box(box_),
    date(date_),
    infos(infos_),
    page(page_),
    event_type(event_type_),
    time(0)
    {
    }
};

struct PageInfo {
  float min_y;
  float max_y;

  PageInfo() :
    min_y(999999999),
    max_y(0)
    {
    }
  PageInfo(const PageInfo& b) :
    min_y(b.min_y),
    max_y(b.max_y)
    {
    }

  PageInfo(float min_y_,
           float max_y_) :
    min_y(min_y_),
    max_y(max_y_)
    {
    }

};

class MyMapCollector : public MapCollector, public std::vector<Element> {
public:
  int page = 1;
  std::map<int, PageInfo> page_infos;

  virtual void Graph2TimeMap( const FloatRect& box, const TimeSegment& dates, const GuidoElementInfos& infos ) {
    PageInfo inf;
    if (page_infos.count(this->page) > 0) {
      inf = page_infos[this->page];
    }
    else {
      page_infos.emplace(this->page, PageInfo());
    }
    inf.min_y = std::min(inf.min_y, box.top);
    inf.max_y = std::max(inf.max_y, box.bottom);
    page_infos[this->page] = PageInfo(inf.min_y, inf.max_y);
    if (infos.type == kNote) {
      // we only play the first staff
      GuidoElementInfos ninfos;
      ninfos.type = infos.type;
      ninfos.staffNum = infos.staffNum;
      ninfos.voiceNum = infos.voiceNum;
      ninfos.midiPitch = infos.midiPitch;
      ninfos.isTied = infos.isTied;
      ninfos.intensity = infos.intensity;

      if (ninfos.staffNum != 1) {
        ninfos.midiPitch = 0;
      }
      this->push_back(Element(box, dates.first, ninfos, this->page, 1)); // note on
      this->push_back(Element(box, dates.second, ninfos, this->page, 2)); // note off
    }
    else if (infos.type == kRest) {
      this->push_back(Element(box, dates.first, infos, this->page, 0));
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
  out.setDenominator(denom * 4);
}

bool erase_tag(std::string& guidostr, std::string tag) {
  size_t start_pos = guidostr.find("\\" + tag + "<\"");
  if (start_pos != string::npos) {
    size_t end_pos = guidostr.find(">", start_pos);
    // std::cout << start_pos << " " << end_pos << std::endl;
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
    // std::cout << out.getNumerator() << " " << out.getDenominator() << std::endl;
    cumul += out;
    float time = parse_float(content, off);
    GuidoDate* date = new GuidoDate();
    date->num = cumul.getNumerator();
    date->denom = cumul.getDenominator();

    date_to_time.push_back(std::pair<GuidoDate*, float>(date, time));
    //std::cout << "Time:" << time
    //<< " @" << cumul.getNumerator() << "/" << cumul.getDenominator()
    //        << std::endl;
  }
}


float to_float(GuidoDate& a) {
  return (float)a.num / (float)a.denom;
}

bool sort_by_date(Element& a, Element& b)
{
  float af = to_float(a.date);
  float bf = to_float(b.date);
  return (af < bf);
}


void interpolate(std::vector<std::pair<GuidoDate*, float> >& date_to_time, MyMapCollector& map_collector) {
  /*
    for (auto it = date_to_time.begin(); it != date_to_time.end(); ++it) {
    auto next = it + 1;
    float duration = 2;

    if (next != date_to_time.end()) {
    duration = next->second - it->second;
    }
    }
  */

  // step is compute with the mapping
  double bps = 3.0;
  auto itrecording = date_to_time.begin();
  double timeoffset = 0;
  bool first = true;

  for (auto it = map_collector.begin(); it != map_collector.end(); ++it) {
    float bdate = to_float(it->date);

    // update itrecording
    auto lnext = itrecording + 1;
    if (first || (bdate >= to_float(*lnext->first))) {
      first = false;
      while ((lnext != date_to_time.end()) && (bdate >= to_float(*lnext->first))) {
        ++itrecording;
        lnext = itrecording + 1;
      }
      if (lnext != date_to_time.end()) {
        float beat_duration = to_float(*lnext->first) - to_float(*itrecording->first);
        float sec_duration = lnext->second - itrecording->second;

        if (sec_duration > 0) {
          bps = beat_duration / sec_duration;
        }
      }
    }
    it->time = itrecording->second + (bdate - to_float(*itrecording->first)) / bps;
    // it->time = lnext->second + (bdate - to_float(*lnext->first)) / bps;

  }
}


int loadsoundfont(fluid_synth_t* synth)
{
  return fluid_synth_sfload(synth, "/usr/share/sounds/sf2/FluidR3_GM.sf2", 1);
}


std::string extract_tag(std::string& content_xml, std::string tag_name) {
  int si_tag = tag_name.size();
  size_t start_pos = content_xml.find("<" + tag_name + ">");
  if(start_pos == std::string::npos)
    return "none";
  size_t end_pos = content_xml.find("</" + tag_name + ">");
  if(end_pos == std::string::npos)
    return "none";
  int si_tag_with_chev = si_tag + 2;
  int beg = start_pos + si_tag_with_chev;
  std::string tag = content_xml.substr(beg, end_pos - beg);
  return tag;
}

int extract_instrument(std::string& content_xml) {
  auto tag = extract_tag(content_xml, "midi-program");
  if (tag == "none")
    return 1;
  return std::atoi(tag.c_str());
}

int extract_transpo(std::string& content_xml) {
  replace(content_xml, "\n", "");
  auto tag = extract_tag(content_xml, "transpose");
  if (tag == "none") return 0;
  auto chromatic = extract_tag(tag, "chromatic");
  if (chromatic == "none") return 0;
  return std::atoi(chromatic.c_str());
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
  layoutSettings.systemsDistribution = kAlwaysDistrib;
  layoutSettings.systemsDistribLimit = 0.25;
  layoutSettings.force = 750;
  layoutSettings.spring = 1.11;
  layoutSettings.neighborhoodSpacing = 0;
  layoutSettings.optimalPageFill = 1;
  layoutSettings.resizePage2Music = 1;
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
  std::ifstream ifs(musicxml_file.c_str());
  std::string content_xml;
  getline(ifs, content_xml, '\0');
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, 1, guido);
  int transpo = extract_transpo(content_xml);
  std::cout << "TRANSPO:" << transpo << std::endl;
  int instru = extract_instrument(content_xml);
  std::cout << "INSTRU:" << instru << std::endl;
  // MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), false, guido);
  std::string guidostr = guido.str();
  erase_tag(guidostr, "instr");
  erase_tag(guidostr, "title");
  erase_tag(guidostr, "composer");
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
  long audio_nframe = 0;
  Time2GraphicMap systemMap;
  bool result = false;
  GuidoOnDrawDesc* main_desc;
  GuidoOnDrawDesc* desc = get_on_draw_desc(currentSession, scoreParameters);;
  CairoDevice* main_device;
  float fps = 24;
  float last_draw = 0;
  MyMapCollector map_collector;

  fluid_settings_t* settings;
  fluid_synth_t* synth;
  fluid_midi_router_t* router;
  fluid_midi_router_rule_t* rule;
  settings = new_fluid_settings();
  synth = new_fluid_synth(settings);
  float sample_rate = 48000;
  float* lout = new float[(int)sample_rate * 180];
  fluid_synth_set_sample_rate(synth, sample_rate);
  fluid_synth_set_gain(synth, 0.25);
  loadsoundfont(synth);
  fluid_synth_program_change(synth, 1, instru - 1);

  auto pFile = fopen("audio.raw", "wb");

  for (int npage = 1; npage <= pageCount; ++npage) {
    map_collector.page = npage;
    GuidoGetMap(gr, npage, width, height, kGuidoEvent, map_collector);
  }
  std::sort(map_collector.begin(), map_collector.end(), sort_by_date);
  interpolate(date_to_time, map_collector);
  int last_page = 0;
  int current_page = 1;
  bool last_tied = false;

  // Loop to generate the video
  for (auto it = map_collector.begin(); it != map_collector.end(); ++it) {
    // int current_page = it->page;
    // We ignore note off events (no need to highlight)
  new_page:
    if (current_page != last_page) {
      last_page = current_page;
      if (current_page > pageCount * 2) {
        std::cout << "Aborted, no more page @"
                  << it->time << "s"
                  << std::endl;
        break;
      }
      PageInfo page_info = map_collector.page_infos[current_page];
      // HERE COMPUTE MARGIN
      std::cout << "CURRENT PAGE:" << current_page << std::endl;
      float page_height = page_info.max_y - page_info.min_y;
      // guidohttpd::guidosession::sDefaultScoreParameters.guidoParameters.pageFormat.margintop = (height / 2.0 - page_height / 2.0);
      // std::cout << "MARGIN TOP:" << height - page_info.max_y << std::endl;
      // currentSession->updateGRH(guidohttpd::guidosession::sDefaultScoreParameters);

      main_desc = get_on_draw_desc(currentSession, scoreParameters);
      main_device = (CairoDevice*)main_desc->hdc;

      main_desc->page = desc->page = current_page;
      err = convert_score_to_png(main_desc, fBuffer);
      if (err != 0) {
        std::cerr << "An error occured 1" << std::endl;
        return 1;
      }
      err = GuidoGetSystemMap(gr, current_page, width, height, systemMap);
      if (err != 0) {
        std::cerr << "An error occured 2" << std::endl;
        return 1;
      }
    }

    // We ignore note off to highlight
    int target_frame = round(it->time * fps);
    int nframe_todraw = target_frame - nframe;
    for (int k = 0; k < nframe_todraw; ++k) {
      ofstream myfile;
      std::string output_file_path = "output" + std::to_string(nframe++) + ".png";
      myfile.open (output_file_path.c_str());
      myfile.write(fBuffer.start_, fBuffer.size_);
      myfile.close();
    }
    if (it->event_type != 2) {
      result = GuidoGetTime(it->date, systemMap, t, r);
      if (!result) {
        std::cerr << "Beat not found changing page" << std::endl;
        ++current_page;
        goto new_page;
        // return 1;
      }
      else {
        err = convert_score_to_png(desc, fBuffer, &r, main_device, sizex, sizey);
        if (err != 0) {
          std::cerr << "An error occured 3" << std::endl;
          return 1;
        }
      }
    }
    // we ignore rests in player
    int midiPitch = it->infos.midiPitch;
    bool should_play = (midiPitch > 0);
    long target_audio_frame = round(it->time * sample_rate);
    long naudio_frame = target_audio_frame - audio_nframe;

    if (it->infos.isTied && last_tied) {
      should_play = false;
    }
    if ((it->event_type == 1) && ((midiPitch > 0)))
      last_tied = it->infos.isTied;
    if (naudio_frame > 0) {
      fluid_synth_write_float(synth, naudio_frame, lout, 0, 1, lout, 0, 1);
      fwrite(lout, 1, naudio_frame * sizeof(float), pFile);
    }
    if (should_play) {
      if (it->event_type == 1)
        fluid_synth_noteon(synth, 1, midiPitch + transpo, 127);
    }
    if (it->event_type == 2)
      fluid_synth_noteoff(synth, 1, midiPitch + transpo);

    audio_nframe = target_audio_frame;
  }

  // we have to generate 2sec of the last image / sounds.
  std::cout << "ALL DONE" << std::endl;
  fclose(pFile);
  return 0;
}
