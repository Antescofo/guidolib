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

  std::cout << "Variable length: " << value << std::endl;
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


bool sort_by_time(Element& a, Element& b)
{
  return (a.time < b.time);
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

  for (auto it : *collector) {
    // it.time
    // it.midiPitch
    float relative_time = it.time - preview_audio_begin;
    float delta_time = relative_time - last_time;
    unsigned long variable_delta_time = division * 2.0 * delta_time; // * bpm or shit like that

    std::cout << std::endl << "EUH BONJOUR: " << it.time << " " << it.event_type << " " << it.infos.midiPitch << std::endl;
    std::cout << relative_time << " - " << last_time << " => " << delta_time << std::endl;
    unsigned char key = it.infos.midiPitch;
    unsigned char velocity = 80;
    if (it.event_type == 1) { // note on
      status = 144;  // 10010000
    }
    else if (it.event_type == 2) { // note off
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
  unsigned short division = 96;

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
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, part_filter, guidostream);
  guido = guidostream.str();
  preclean_guido(guido);

  return get_map_collector_from_guido(guido, date_to_time);
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


