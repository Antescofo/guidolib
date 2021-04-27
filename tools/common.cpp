#include "common.hh"
#include <cstring>
#include <algorithm>
#include <string>
#include <libmusicxml/libmusicxml.h>

int parse_int(std::string& content, int& off) {
  int num = 0;
  while ((off < content.size()) && ((content[off] >= '0') && (content[off] <= '9'))) {
    num = num * 10 + (content[off] - '0');
    ++off;
  }
  return num;
}

bool sort_by_time(Element& a, Element& b)
{
  if (fabs(a.time - b.time) < 0.00001) {
    return (a.event_type > b.event_type);
  }
  return (a.time < b.time);
}


bool sort_by_date(Element& a, Element& b)
{
  float af = to_float(a.date);
  float bf = to_float(b.date);
  /*
  if (abs(af - bf) < 0.00001) {
    if (a.event_type == 2) return 1;
    return 0;
  }
  */
  return (af < bf);
}

float parse_float(std::string& content, int& off) {
  float num = 0;
  while ((off < content.size() && (content[off] == ' '))) ++off;
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
  float denom = 1;

  float num = parse_float(content, off);
  bool has_denom = false;
  if (content[off] == '/') {
    ++off;
    denom = parse_float(content, off);
    has_denom = true;
  }
  if (has_denom) {
    out.setNumerator(num);
    out.setDenominator(denom * 4);
  }
  else {
    out.setNumerator(num * 4 * 2 * 3 * 2);
    out.setDenominator(4 * 4 * 2 * 3 * 2);
  }
  return true;
}


static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}
bool parse_asco(const std::string& asco_file, std::vector<std::pair<GuidoDate*, float> >& date_to_time) {
  std::ifstream ifs(asco_file.c_str());
  std::string content;
  getline(ifs, content, '\0');
  // content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());
  replace(content, "\t", " ");
  replace(content, "\n", " ");
  replace(content, ",", " ");
  replace(content, "  ", " ");
  replace(content, "} }", "}}");

  size_t start_pos_metadata = content.find("<metadata>");
  size_t end_pos_metadata = content.find("</metadata>");

  if ((start_pos_metadata != std::string::npos) && (end_pos_metadata != std::string::npos)) {
    content.erase(start_pos_metadata, end_pos_metadata - start_pos_metadata);
  }
  size_t start_pos = content.find("@eval_when_load { $nim := NIM {");

  start_pos = content.find("NIM {", start_pos) + 5;
  size_t end_pos = content.find("}}");

  content = content.substr(start_pos, end_pos - start_pos);
  // BEAT TIME BEAT TIME BEAT TIME
  int off = 0;
  Fraction cumul;
  replace(content, ")", " ");
  replace(content, "(", " ");
  replace(content, "  ", " ");

  content = ltrim(content);

  /*
  if (content[off] != '(') {
    parse_float(content, off);
  }
  */
  //replace(content, " ", "");

  while (off < content.size()) {
    while ((off < content.size() && (content[off] == ' '))) ++off;
    Fraction out;
    parse_guido_date(content, off, out);
    // std::cout << out.getNumerator() << " " << out.getDenominator() << std::endl;
    cumul += out;
    float time = parse_float(content, off);
    GuidoDate* date = new GuidoDate();
    date->num = cumul.getNumerator();
    date->denom = cumul.getDenominator();
    if ((date_to_time.size() == 0) && (date->num != 0)) {
      GuidoDate* fdate = new GuidoDate();
      fdate->num = 0;
      fdate->denom = 4;
      date_to_time.push_back(std::pair<GuidoDate*, float>(fdate, 0));

    }
    date_to_time.push_back(std::pair<GuidoDate*, float>(date, time));
    //std::cout << "Time:" << time
    //<< " @" << cumul.getNumerator() << "/" << cumul.getDenominator()
    //        << std::endl;
    while ((off < content.size() && (content[off] == ' '))) ++off;
  }
  return true;
}


bool replace(std::string& str, const std::string& from, const std::string& to, bool only_once) {
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


void interpolate(std::vector<std::pair<GuidoDate*, float> >& date_to_time, MyMapCollector& map_collector) {
  double bps = 3.0;
  auto itrecording = date_to_time.begin();
  double timeoffset = 0;
  bool first = true;
  long last_measure = 0;
  double last_time = 0.0;
  double last_bdate = 0.0;
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

    if (it->time < 0) {
      std::cout << "Negative time, something wents wrong with the asco: "
                << it->time << " " << itrecording->second << " " << bdate << " " << to_float(*itrecording->first) << " " << bps << std::endl;
      throw "HOY";
    }
    /*
    std::cout << "OH:"
              << it->event_type << " "
              << it->time << " "
              << it->date << " " << bdate << " " << it->measure << std::endl;
    if (it->measure < last_measure) {
      std::cout << "SHOULD NOT GO BEHIND MEASURE" << std::endl;
      throw "huhuhu";
    }
    if (it->time < last_time) {
      std::cout << "SHOULD NOT GO BEHIND TIME" << std::endl;
      throw "huhuhu";
    }
    if (bdate < last_bdate) {
      std::cout << "SHOULD NOT GO BEHIND BDATE" << std::endl;
      throw "huhuhu";
    }
    */

    last_bdate = bdate;
    last_measure = it->measure;
    last_time = it->time;

  }
}

float to_float(GuidoDate& a) {
  return (float)a.num / (float)a.denom;
}

MyMapCollector* get_map_collector_from_guido(std::string& guido,
                                             std::vector<std::pair<GuidoDate*, float> >& date_to_time) {
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
  register unsigned long buffer;
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
  char* track_data = new char[1024*1024*10];
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

  std::sort(collector->begin(), collector->end(), sort_by_time);
  std::map<unsigned char, bool> last_tieds;
  int transpo = collector->transpo;
  double cumul_delta_time = 0;
  for (auto it : *collector) {
    // it.time
    // it.midiPitch
    double relative_time = it.time - preview_audio_begin;
    double delta_time = relative_time - last_time;
    unsigned long variable_delta_time = division * 2.0 * delta_time; // * bpm or shit like that
    double real_target = division * 2.0 * relative_time;
    double error = real_target - (cumul_delta_time + variable_delta_time);
    double casted_error = floor(error);
    variable_delta_time += casted_error;
    // std::cout << std::endl << "EUH BONJOUR: " << it.time << " " << it.event_type << " " << it.infos.midiPitch << std::endl;
    // std::cout << relative_time << " - " << last_time << " => " << delta_time << std::endl;
    unsigned char key = it.infos.midiPitch + transpo;
    unsigned char velocity = it.infos.intensity;
    if (it.event_type == 1) { // note on
      bool should_play = true;
      bool last_tied = false;
      if (last_tieds.count(key) > 0)
        last_tied = last_tieds[key];
      if (it.infos.isTied && last_tied) {
        should_play = false;
      }
      last_tieds[key] = it.infos.isTied && it.infos.isOriginTied;
      if (!should_play) {
        /*
        std::cout << "NOTE ON~: "
                  << "measure:" << it.measure << " "
                  << "key:" << (int)key << " "
                  << "t:" << it.time << " "
                  << "dt:" << delta_time << " "
                  << std::endl;
        */
        continue;
      }
      /*
      std::cout << "NOTE ON : "
                << "measure:" << it.measure << " "
                << "key:" << (int)key << " "
                << "t:" << it.time << " "
                << "dt:" << delta_time << " "
                << std::endl;
      */
      status = 144;  // 10010000
    }
    else if (it.event_type == 2) { // note off
      /*
      std::cout << "NOTE OFF: "
                << "measure:" << it.measure << " "
                << "key:" << (int)key << " "
                << "t:" << it.time << " "
                << "dt:" << delta_time << " "
                << std::endl;
      */
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
    cumul_delta_time += variable_delta_time;
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
  delete[] track_data;
}


void generate_midi(MyMapCollector* collector, const std::string& output_midi_file, float preview_audio_begin) {
  char* output = new char[1024*1024*10];
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
  unsigned short division = 96 * 2;

  append_value(header_length, output, offset);
  append_value(format, output, offset);
  append_value(ntracks, output, offset);
  append_value(division, output, offset);


  append_track(collector, output, offset, preview_audio_begin, division);
  std::ofstream* outfile = new std::ofstream(output_midi_file.c_str(), std::ofstream::binary);
  outfile->write(output, offset);
  outfile->close();
  delete[] output;
  delete outfile;
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


int extract_transpo(std::string& content_xml) {
  replace(content_xml, "\n", "");
  auto tag = extract_tag(content_xml, "transpose");
  if (tag == "none") return 0;
  auto chromatic = extract_tag(tag, "chromatic");
  if (chromatic == "none") return 0;
  return std::atoi(chromatic.c_str());
}

MyMapCollector* get_map_collector_from_xml_and_asco(const std::string& musicxml_file,
                                                    const std::string& asco_file,
                                                    int part_filter) {
  std::vector<std::pair<GuidoDate*, float> > date_to_time;
  std::ifstream ifs(musicxml_file.c_str());
  std::string content_xml;
  std::stringstream guidostream;
  std::string guido;

  getline(ifs, content_xml, '\0');
  parse_asco(asco_file, date_to_time);
  int transpo = extract_transpo(content_xml);
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, part_filter, guidostream);
  guido = guidostream.str();
  preclean_guido(guido);
  MyMapCollector* ret = get_map_collector_from_guido(guido, date_to_time);
  ret->transpo = transpo;
  return ret;
}


bool erase_tag(std::string& guidostr, std::string tag) {
  size_t start_pos = guidostr.find("\\" + tag + "<\"");
  if (start_pos != std::string::npos) {
    size_t end_pos = guidostr.find(">", start_pos);
    // std::cout << start_pos << " " << end_pos << std::endl;
    guidostr.erase(start_pos, 1 + end_pos - start_pos);
    return true;
  }
  return false;
}

std::string preclean_guido(std::string& guido) {
  erase_tag(guido, "instr");
  erase_tag(guido, "title");
  erase_tag(guido, "composer");
  return guido;
}
