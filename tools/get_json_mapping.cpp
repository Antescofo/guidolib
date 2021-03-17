#include <map>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include "GUIDO2Midi.h"
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
  return true;
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
  return true;
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
      voice_to_measure[infos.voiceNum] = 1;
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

std::string preclean_guido(std::string& guido) {
  erase_tag(guido, "instr");
  erase_tag(guido, "title");
  erase_tag(guido, "composer");
  return guido;
}

std::string base64_guido(std::string& guido) {
  replace(guido, "\n", " ");
  replace(guido, "\r", " ");
  replace(guido, "\t", " ");
  replace(guido, "  ", " ");
  return macaron::Base64().Encode(guido);
}

MyMapCollector* get_map_collector_from_guido(std::string& guido,
                                             std::vector<std::pair<GuidoDate*, float> >& date_to_time,
                                             std::string& outmidi) {
  MyMapCollector* map_collector = new MyMapCollector();
  std::string svg_font_file = "/app/src/guido2.svg";
  guidohttpd::guidosession* currentSession = new guidohttpd::guidosession(svg_font_file, guido, "1shauishauis.gmm");
  // currentSession->updateGRH(guidohttpd::guidosession::sDefaultScoreParameters);

  CGRHandler gr = currentSession->getGRHandler();

  int pageCount = GuidoGetPageCount(gr);


  int width = 1280;
  int height = 720;


  for (int npage = 1; npage <= pageCount; ++npage) {
    map_collector->page = npage;
    GuidoGetMap(gr, npage, width, height, kGuidoBarAndEvent, *map_collector);
  }
  std::sort(map_collector->begin(), map_collector->end(), sort_by_date);

  interpolate(date_to_time, *map_collector);
  guidohttpd::guidosessionresponse return_midi = currentSession->genericReturnMidi();
  if ((return_midi.fHttpStatus != 200) && (return_midi.fHttpStatus != 201)) throw std::invalid_argument("Error convert midi file");
  outmidi = std::string(return_midi.fData, return_midi.fSize);
  return map_collector;
}


int is_big_endian(void)
{
  union {
    uint32_t i;
    char c[4];
  } e = { 0x01000000 };

  return e.c[0];
}

template <class T>
void append_value(T& val, char* output, unsigned long& offset) {
  int bytes = sizeof(val);
// We have to store values big endian style
  if (!is_big_endian()) {
    char* source = (char*)&val;

    for (int k = 0; k < bytes; ++k) {
      output[offset + k] = source[bytes - 1 - k];
    }
  }
  else {
    memcpy(&output[offset], &val, bytes);
  }
  offset += bytes;
}

void append_variable_length(unsigned long value, char* output, unsigned long& offset)
{
  unsigned long buffer;
  buffer = value & 0x7F;

  while ((value >>= 7))
  {
    buffer <<= 8;
    buffer |= ((value & 0x7F) | 0x80);
  }

  while (true)
  {
    output[offset++] = ((unsigned char*)&buffer)[0];
    if (buffer & 0x80)
      buffer >>= 8;
    else
      break;
  }
}

void append_track(MyMapCollector* collector, char* output, unsigned long& offset, float preview_audio_begin, unsigned long division) {
  output[offset++] = 'M';
  output[offset++] = 'T';
  output[offset++] = 'r';
  output[offset++] = 'k';

  // Append events

  // Delta time in append_variable_length
  // Event
  // <MTrk event> = <delta-time><event>
  // <event> = <MIDI event> | <sysex event> | <meta-event>
  // http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html#BMA1_

  float last_time = 0.0;
  char* track_data = new char[10000];
  unsigned long offset_track = 0;
  unsigned char status;

  // We first set the program and the tempo
  append_variable_length(0, track_data, offset_track);
  status = 0xC0;  // program change
  append_value(status, track_data, offset_track);
  unsigned char program = 0;
  append_value(program, track_data, offset_track);


  // Tempo change
  append_variable_length(0, track_data, offset_track);
  status = 0xFF;
  append_value(status, track_data, offset_track);
  status = 0x51;
  append_value(status, track_data, offset_track);
  status = 0x03;
  append_value(status, track_data, offset_track);
  // tt tt tt 500000

  status = 0x07;
  append_value(status, track_data, offset_track);
  status = 0xa1;
  append_value(status, track_data, offset_track);
  status = 0x20;
  append_value(status, track_data, offset_track);

  /*
    This event indicates a tempo change. Another way of putting "microseconds per quarter-note" is "24ths of a microsecond per MIDI clock". Representing tempos as time per beat instead of beat per time allows absolutely exact long-term synchronisation with a time-based sync protocol such as SMPTE time code or MIDI time code. The amount of accuracy provided by this tempo resolution allows a four-minute piece at 120 beats per minute to be accurate within 500 usec at the end of the piece. Ideally, these events should only occur where MIDI clocks would be located -- this convention is intended to guarantee, or at least increase the likelihood, of compatibility with other synchronisation devices so that a time signature/tempo map stored in this format may easily be transferred to another device.
  */
  for (auto it : *collector) {
    // it.time
    // it.midiPitch
    float relative_time = it.time - preview_audio_begin;
    float delta_time = relative_time - last_time;
    unsigned long variable_delta_time = division * 2.0 * delta_time; // * bpm or shit like that
    unsigned char key = it.infos.midiPitch;
    unsigned char velocity = 80;
    if (it.event_type == 1) { // note on
      std::cout << variable_delta_time << std::endl;
      std::cout << "NOTEON " << it.infos.midiPitch << " " << it.time << " " << it.measure << std::endl;
      status = 144;  // 10010000
    }
    else if (it.event_type == 2) { // note off
      std::cout << variable_delta_time << std::endl;
      
      std::cout << "NOTEOFF " << it.infos.midiPitch << " " << it.time << std::endl;
      std::cout << std::endl;
      status = 128; // 10000000
    }
    else {
      continue;
    }
    append_variable_length(variable_delta_time, track_data, offset_track);
    append_value(status, track_data, offset_track);
    append_value(key, track_data, offset_track);
    append_value(velocity, track_data, offset_track);
    last_time = relative_time;
  }
  // end of track
  append_variable_length(0, track_data, offset_track);
  status = 0xff;
  append_value(status, track_data, offset_track);
  status = 0x2f;
  append_value(status, track_data, offset_track);
  status = 0x00;
  append_value(status, track_data, offset_track);

  unsigned int track_length = offset_track;
  append_value(track_length, output, offset);
  memcpy(&output[offset], track_data, offset_track);
  offset += offset_track;
}

std::string generate_midi(MyMapCollector* collector, float preview_audio_begin) {
  char* output = new char[1024*1024];
  unsigned long offset = 0;

  output[offset++] = 'M';
  output[offset++] = 'T';
  output[offset++] = 'h';
  output[offset++] = 'd';
  unsigned int header_length = 6;
  unsigned short format = 0;
  unsigned short ntracks = 1;
  // unsigned short division = 59176; // many Pulses (i.e. clocks) Per Quarter Note (abbreviated as PPQN)
  // unsigned short division = 42000; // many Pulses (i.e. clocks) Per Quarter Note (abbreviated as PPQN)
  unsigned short division = 96;

  append_value(header_length, output, offset);
  append_value(format, output, offset);
  append_value(ntracks, output, offset);
  append_value(division, output, offset);

  append_track(collector, output, offset, preview_audio_begin, division);
  return std::string(output, offset);
}

int main(int argc, char* argv[]) {
  if (argc < 6) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file asco_file part_filter begin_bar end_bar"
         << endl;
    return 1;
  }

  bool whole_guido_too = false;
  bool export_debug = false;
  if (argc >= 7) {
    std::string addparam = argv[6];
    if (addparam == "--whole") {
      whole_guido_too = true;
    }
    else if (addparam == "--debug") {
      export_debug = true;
    }
  }
  if (argc >= 8) {
    std::string addparam = argv[7];
    if (addparam == "--whole") {
      whole_guido_too = true;
    }
    else if (addparam == "--debug") {
      export_debug = true;
    }
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
  if (!has_begin_bar) begin_bar = 0;
  if (!has_end_bar) end_bar = 0;
  std::ostringstream preview_guido_stream;
  MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), true, begin_bar, end_bar, part_filter, preview_guido_stream);
  std::string preview_guido = preview_guido_stream.str();
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, part_filter, guido);
  std::string guidostr = guido.str();
  // We can post process things here we do not want in th guido
  preclean_guido(guidostr);

  std::string out_midi_whole;
  MyMapCollector* map_collector = get_map_collector_from_guido(guidostr, date_to_time, out_midi_whole);

  std::string whole_guido = guidostr;
  int num_offset_preview = 0;
  int deno_offset_preview = 1;
  float preview_audio_begin = 0;
  int num_preview_end = 0;
  int deno_preview_end = 0;
  float preview_audio_end = 0;

  int computed_end_bar = 99999;
  int computed_begin_bar = 1;
  if (has_begin_bar) computed_begin_bar = begin_bar;
  if (has_end_bar) computed_end_bar = end_bar;

  if (has_begin_bar) {
    for (auto it : *map_collector) {
      if (it.voice <= 0) continue;

      if (it.event_type != 2) {
        if ((it.measure <= computed_end_bar) && (it.measure >= computed_begin_bar)) {
          preview_audio_begin = it.time;
          num_offset_preview = it.date.num;
          deno_offset_preview = it.date.denom;
          break;
        }
      }
    }
  }

  for (auto it : *map_collector) {
    if (it.voice <= 0) continue;
    if ((it.measure <= computed_end_bar) && (it.measure >= computed_begin_bar)) {
      // std::cout << "OH:" << it.measure << " " << it.time << " " << it.event_type << " " << it.staff << " " << it.voice << std::endl;
      preview_audio_end = it.time;
      num_preview_end = it.date.num;
      deno_preview_end = it.date.denom;
    }
  }

  // Filter it with begin_bar & end_bar
  std::string ret = "{";
  bool first = true;

  ret += "\"preview_beat_mapping\": {";
  first = true;
  std::vector<std::pair<GuidoDate*, float> > preview_date_to_time;
  auto gdate = new GuidoDate();
  gdate->num = 0;
  gdate->denom = 1;
  preview_date_to_time.push_back(std::pair<GuidoDate*, float>(gdate, preview_audio_begin));

  float guido_time_start = (float)num_offset_preview / (float)deno_offset_preview;
  float guido_time_end = (float)num_preview_end / (float)deno_preview_end;
  Fraction begin_offset;
  begin_offset.setNumerator(num_offset_preview);
  begin_offset.setDenominator(deno_offset_preview);

  Fraction end_preview;
  end_preview.setNumerator(num_preview_end);
  end_preview.setDenominator(deno_preview_end);
  auto offset = end_preview - begin_offset;

  for (auto it : date_to_time) {
    if ((to_float(*it.first) >= guido_time_start) && (to_float(*it.first) <= guido_time_end)) {
      gdate = new GuidoDate();
      Fraction current;
      current.setNumerator(it.first->num);
      current.setDenominator(it.first->denom);
      auto result = current - begin_offset;
      gdate->num = result.getNumerator();
      gdate->denom = result.getDenominator();
      preview_date_to_time.push_back(std::pair<GuidoDate*, float>(gdate, it.second));
    }
  }
  gdate = new GuidoDate();
  gdate->num = offset.getNumerator();
  gdate->denom = offset.getDenominator();
  preview_date_to_time.push_back(std::pair<GuidoDate*, float>(gdate, preview_audio_end));
  std::string out_midi_preview;
  MyMapCollector* preview_map_collector = get_map_collector_from_guido(preview_guido, preview_date_to_time, out_midi_preview);

  for (auto it : *preview_map_collector) {
    // preview_audio_begin = std::min(preview_audio_begin, it.time);
    // preview_audio_end = std::max(preview_audio_end, it.time);
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

  if (whole_guido_too) {
    ret += "\"beat_mapping\": {";
    first = true;
    for (auto it : *map_collector) {
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
  }

  ret += ", \"num_offset_preview\": " + to_string(num_offset_preview);
  ret += ", \"deno_offset_preview\": " + to_string(deno_offset_preview);
  ret += ", \"preview_audio_begin\": " + to_string(preview_audio_begin);

  ret += ", \"num_preview_end\": " + to_string(num_preview_end);
  ret += ", \"deno_preview_end\": " + to_string(deno_preview_end);
  ret += ", \"preview_audio_end\": " + to_string(preview_audio_end);

  ret += ", \"preview_midi\": \"" + macaron::Base64().Encode(generate_midi(preview_map_collector, preview_audio_begin)) + "\"";

  ret += ", \"preview_guido_b64\": \"" + base64_guido(preview_guido) + "\"";
  if (whole_guido_too)
    ret += ", \"guido_b64\": \"" + base64_guido(guidostr) + "\"";

  ret += "}";
  if (export_debug) {
    cout << "export const canon = ";
  }

  cout << ret << endl;

  return 0;
}
