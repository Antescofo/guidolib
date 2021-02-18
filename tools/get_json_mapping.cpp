#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include "guidosession.h"
#include <libmusicxml/libmusicxml.h>
#include "engine.h"
#include "Fraction.h"
#include "Base64.h"
#include <cmath>


using namespace std;


bool replace(std::string& str, const std::string& from, const std::string& to, bool only_once=false) {
  int last_pos = 0;
  while (1) {
    size_t start_pos = str.find(from, last_pos);
    if(start_pos == std::string::npos)
      return false;
    if (only_once)
      last_pos = start_pos + 2;
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

bool parse_asco(std::string& asco_file, std::vector<std::pair<GuidoDate*, float> >& date_to_time) {
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

float to_float(GuidoDate& a) {
  return (float)a.num / (float)a.denom;
}


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

struct Element {
  FloatRect box;
  GuidoDate date;
  // TimeSegment dates;
  GuidoElementInfos infos;
  int event_type; // 0 for rest, 1 for note on, 2 for note off
  int page;
  int measure;
  float time;

  Element(const FloatRect& box_, const GuidoDate& date_, const GuidoElementInfos& infos_, int page_, int event_type_, int measure_) :
    box(box_),
    date(date_),
    infos(infos_),
    page(page_),
    event_type(event_type_),
    time(0),
    measure(measure_)
    {
    }
};

bool sort_by_date(Element& a, Element& b)
{
  float af = to_float(a.date);
  float bf = to_float(b.date);
  if (abs(af - bf) < 0.00001) {
    if (a.event_type == 2) return 1;
    return 0;
  }
  return (af < bf);
}


class MyMapCollector : public MapCollector, public std::vector<Element> {
public:
  int page = 1;
  int measure = 1;
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
    if (infos.type == kBar) {
      ++measure;
    }
    else if (infos.type == kNote) {
      // we only play the first staff
      GuidoElementInfos ninfos;

      ninfos.type = infos.type;
      ninfos.staffNum = infos.staffNum;
      ninfos.voiceNum = infos.voiceNum;
      ninfos.midiPitch = infos.midiPitch;
      ninfos.isTied = infos.isTied;
      ninfos.isOriginTied = infos.isOriginTied;
      ninfos.intensity = infos.intensity;
      this->push_back(Element(box, dates.first, ninfos, this->page, 1, measure)); // note on
      bool noteoff = ((!ninfos.isTied) || (!ninfos.isOriginTied));
      if (noteoff) {
        this->push_back(Element(box, dates.second, ninfos, this->page, 2, measure)); // note off
      }
    }
    else if (infos.type == kRest) {
      this->push_back(Element(box, dates.first, infos, this->page, 0, measure));
    }
  }
};


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
    if (first || ((lnext != date_to_time.end()) && (bdate >= to_float(*lnext->first)))) {
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

int main(int argc, char* argv[]) {
  if (argc < 6) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file asco_file part_filter begin_bar end_bar"
         << endl;
    return 1;
  }

  string musicxml_file = argv[1];
  string asco_file = argv[2];
  int part_filter = stoi(argv[3]);
  string begin_bar_str = argv[4];
  string end_bar_str = argv[5];
  int begin_bar = stoi(begin_bar_str);
  int end_bar = stoi(end_bar_str);
  bool has_begin_bar = (begin_bar > 1);
  bool has_end_bar = (end_bar > 0);
  guidohttpd::makeApplication(argc, argv);
  guidohttpd::startEngine();

  std::vector<std::pair<GuidoDate*, float> > date_to_time;

  parse_asco(asco_file, date_to_time);


  std::ifstream ifs(musicxml_file.c_str());
  std::string content_xml;
  std::stringstream guido;

  getline(ifs, content_xml, '\0');

  /*
     if ((beginMeasure != 0) || (endMeasure != 0)) {
            return partialxml2guido(xmlfile, generateBars, partFilter, beginMeasure, endMeasure, out, 0);
        }
                return xml2guido(xmlfile, generateBars, partFilter, out, file);
        }

   */
  // musicxmlfile2guido(const char *file, bool generateBars, int beginMeasure, int endMeasure, int partFilter, ostream& out)

  if (!has_begin_bar) begin_bar = 0;
  if (!has_end_bar) end_bar = 0;
  std::ostringstream preview_guido_stream;
  MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), true, begin_bar, end_bar, part_filter, preview_guido_stream);
  std::string preview_guido = preview_guido_stream.str();
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, part_filter, guido);
  std::string guidostr = guido.str();
  // We can post process things here we do not want in th guido
  erase_tag(guidostr, "instr");
  erase_tag(guidostr, "title");
  erase_tag(guidostr, "composer");

  MyMapCollector map_collector;
  std::string svg_font_file = "/app/src/guido2.svg";
  guidohttpd::guidosession* currentSession = new guidohttpd::guidosession(svg_font_file, guidostr, "1shauishauis.gmm");
  // currentSession->updateGRH(guidohttpd::guidosession::sDefaultScoreParameters);

  CGRHandler gr = currentSession->getGRHandler();
  int pageCount = GuidoGetPageCount(gr);
  int width = 1280;
  int height = 720;


  for (int npage = 1; npage <= pageCount; ++npage) {
    map_collector.page = npage;
    GuidoGetMap(gr, npage, width, height, kGuidoBarAndEvent, map_collector);
  }
  std::sort(map_collector.begin(), map_collector.end(), sort_by_date);

  interpolate(date_to_time, map_collector);

  std::string whole_guido = guidostr;
  int num_offset_preview = 0;
  int deno_offset_preview = 1;
  double preview_audio_begin = 0;
  int num_preview_end = 0;
  int deno_preview_end = 0;
  double preview_audio_end = 0;

  int computed_end_bar = 99999;
  if (has_end_bar) computed_end_bar = end_bar;

  if (has_begin_bar) {
      for (auto it : map_collector) {
        if (it.event_type != 2) {
          if (it.measure >= begin_bar) {
            preview_audio_begin = it.time;
            num_offset_preview = it.date.num;
            deno_offset_preview = it.date.denom;
            break;
          }
        }
      }
    // std::cout << "Need to compute" << endl;
    // return 1;
  }

  for (auto it : map_collector) {
    preview_audio_end = it.time;
    num_preview_end = it.date.num;
    deno_preview_end = it.date.denom;
    if (it.measure > computed_end_bar) {
      break;
    }
  }

  // Filter it with begin_bar & end_bar
  std::string ret = "{";
  ret += "\"beat_mapping\": {";
  bool first = true;
  for (auto it : map_collector) {
    if (it.event_type != 2) {
      if (!first) ret += ", ";
      first = false;
      ret += "\"" + to_string(it.date.num) + "/" + to_string(it.date.denom) + "\"";
      ret += ": {";
      ret += "\"t\": " + to_string(it.time);
      ret += ", \"m\": " + to_string(it.measure);
      ret += ", \"e\": " + to_string(it.event_type);
      ret += "}";

    }
  }
  ret += "}";
  ret += ", \"num_offset_preview\": " + to_string(num_offset_preview);
  ret += ", \"deno_offset_preview\": " + to_string(deno_offset_preview);
  ret += ", \"preview_audio_begin\": " + to_string(preview_audio_begin);

  ret += ", \"num_preview_end\": " + to_string(num_preview_end);
  ret += ", \"deno_preview_end\": " + to_string(deno_preview_end);
  ret += ", \"preview_audio_end\": " + to_string(preview_audio_end);


  // std::string preview_guido = guidostr;
  replace(preview_guido, "\n", " ");
  replace(preview_guido, "\r", " ");
  replace(preview_guido, "\t", " ");

  replace(preview_guido, "  ", " ");
  ret += ", \"preview_guido_b64\": \"" + macaron::Base64().Encode(preview_guido) + "\"";

  ret += "}";
  cout << ret << endl;
  return 0;
}
