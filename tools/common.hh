#ifndef COMMON_HH_
# define COMMON_HH_
# include <vector>
# include "guidosession.h"
# include "Fraction.h"
# include <cmath>


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
  int staff;
  int voice;
  Element(const FloatRect& box_, const GuidoDate& date_, const GuidoElementInfos& infos_, int page_, int event_type_, int measure_, int staff_, int voice_) :
    box(box_),
    date(date_),
    infos(infos_),
    page(page_),
    event_type(event_type_),
    time(0),
    measure(measure_),
    staff(staff_),
    voice(voice_)
    {
    }
};


class MyMapCollector : public MapCollector, public std::vector<Element> {
public:
  int transpo = 0;
  int page = 1;
  int min_measure = 1;
  std::map<int, PageInfo> page_infos;
  std::map<int, int> voice_to_measure;
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
    if (voice_to_measure.find(infos.voiceNum) == voice_to_measure.end()) {
      voice_to_measure[infos.voiceNum] = min_measure;
    }
    if (infos.type == kBar) {
      voice_to_measure[infos.voiceNum] += 1;
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
      this->push_back(Element(box, dates.first, ninfos, this->page, 1, voice_to_measure[infos.voiceNum], infos.staffNum, infos.voiceNum)); // note on
      bool noteoff = ((!ninfos.isTied) || (!ninfos.isOriginTied));
      if (noteoff) {
        this->push_back(Element(box, dates.second, ninfos, this->page, 2, voice_to_measure[infos.voiceNum], infos.staffNum, infos.voiceNum)); // note off
      }
    }
    else if (infos.type == kRest) {
      this->push_back(Element(box, dates.first, infos, this->page, 0, voice_to_measure[infos.voiceNum], infos.staffNum, infos.voiceNum));
    }
  }
};

bool parse_asco(const std::string& asco_file, std::vector<std::pair<GuidoDate*, float> >& date_to_time);
bool replace(std::string& str, const std::string& from, const std::string& to, bool only_once=false);
void interpolate(std::vector<std::pair<GuidoDate*, float> >& date_to_time, MyMapCollector& map_collector);
float to_float(GuidoDate& a);
MyMapCollector* get_map_collector_from_guido(std::string& guido,
                                             std::vector<std::pair<GuidoDate*, float> >& date_to_time);
MyMapCollector* get_map_collector_from_xml_and_asco(const std::string& musicxml_file,
                                                    const std::string& asco_file,
                                                    int part_filter);

void generate_midi(MyMapCollector* collector, const std::string& outfile, float preview_audio_begin = 0);
std::string preclean_guido(std::string& guido);
#endif
