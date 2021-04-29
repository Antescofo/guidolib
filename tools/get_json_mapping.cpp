#include <map>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <libmusicxml/libmusicxml.h>
#include "engine.h"
#include "Base64.h"
#include "common.hh"

using namespace std;


std::string base64_guido(std::string& guido) {
  replace(guido, "\n", " ");
  replace(guido, "\r", " ");
  replace(guido, "\t", " ");
  replace(guido, "  ", " ");
  return macaron::Base64().Encode(guido);
}

int main(int argc, char* argv[]) {
  if (argc < 7) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file asco_file part_filter begin_bar end_bar max_sec"
         << endl;
    return 1;
  }

  bool whole_guido_too = false;
  bool export_debug = false;
  if (argc >= 8) {
    std::string addparam = argv[6];
    if (addparam == "--whole") {
      whole_guido_too = true;
    }
    else if (addparam == "--debug") {
      export_debug = true;
    }
  }
  if (argc >= 9) {
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
  int max_sec = stoi(argv[6]);
  int begin_bar = stoi(begin_bar_str);
  int end_bar = stoi(end_bar_str);
  guidohttpd::makeApplication(argc, argv);
  guidohttpd::startEngine();

  std::vector<std::pair<GuidoDate*, float> > date_to_time;

  parse_asco(asco_file, date_to_time);

  std::ifstream ifs(musicxml_file.c_str());
  std::string content_xml;
  std::stringstream guido;

  getline(ifs, content_xml, '\0');
  MusicXML2::musicxmlstring2guidoOnPart(content_xml.c_str(), true, part_filter, guido);
  std::string guidostr = guido.str();
  // We can post process things here we do not want in th guido
  preclean_guido(guidostr);

  MyMapCollector* map_collector = get_map_collector_from_guido(guidostr, date_to_time);
  std::string whole_guido = guidostr;
  int num_offset_preview = 0;
  int deno_offset_preview = 1;
  float preview_audio_begin = 0;
  int num_preview_end = 0;
  int deno_preview_end = 0;
  float preview_audio_end = 0;

  int computed_end_bar = 99999;
  int computed_begin_bar = begin_bar;
  bool has_end_bar = false;
  if (end_bar > 0) {
    computed_end_bar = end_bar;
    has_end_bar = true;
  }
  for (auto it : *map_collector) {
    if (it.voice <= 0) continue;
    if (it.event_type != 2) {
      if ((it.measure <= computed_end_bar) && (it.measure >= computed_begin_bar)) {
        preview_audio_begin = it.time;
        begin_bar = it.measure;
        num_offset_preview = it.date.num;
        deno_offset_preview = it.date.denom;
        break;
      }
    }
  }
  for (auto it : *map_collector) {
    if (it.voice <= 0) continue;
    if ((it.measure <= computed_end_bar) && (it.measure >= computed_begin_bar) && (has_end_bar || (it.measure == end_bar) || ((it.time - preview_audio_begin) <= max_sec))) {
      // std::cout << "OH:" << it.measure << " " << it.time << " " << it.event_type << " " << it.staff << " " << it.voice << std::endl;
      preview_audio_end = it.time;
      end_bar = it.measure;
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
  std::ostringstream preview_guido_stream;
  MusicXML2::musicxmlfile2guido(musicxml_file.c_str(), true, begin_bar, end_bar, part_filter, preview_guido_stream);
  std::string preview_guido = preview_guido_stream.str();
  MyMapCollector* preview_map_collector = get_map_collector_from_guido(preview_guido, preview_date_to_time);

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
  // We add half sec margin
  preview_audio_begin -= 0.5;
  preview_audio_end += 0.5;
  if (preview_audio_begin < 0) preview_audio_begin = 0;

  ret += ", \"num_offset_preview\": " + to_string(num_offset_preview);
  ret += ", \"deno_offset_preview\": " + to_string(deno_offset_preview);
  ret += ", \"preview_audio_begin\": " + to_string(preview_audio_begin);
  ret += ", \"preview_end_bar\": " + to_string(end_bar);
  ret += ", \"preview_begin_bar\": " + to_string(begin_bar);

  ret += ", \"num_preview_end\": " + to_string(num_preview_end);
  ret += ", \"deno_preview_end\": " + to_string(deno_preview_end);
  ret += ", \"preview_audio_end\": " + to_string(preview_audio_end);

  // ret += ", \"preview_midi\": \"" + macaron::Base64().Encode(generate_midi(preview_map_collector, preview_audio_begin)) + "\"";

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
