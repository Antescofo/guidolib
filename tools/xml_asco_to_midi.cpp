#include <map>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <stdexcept>
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
#include "common.hh"


using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 6) {
    cerr << "Usage: "
         << argv[0]
         << " musicxml_file asco_file transpo part_filter output_midi_file"
         << endl;
    return 1;
  }
  string musicxml_file = argv[1];
  string asco_file = argv[2];
  int transpo = stoi(argv[3]);
  int part_filter = stoi(argv[4]);
  string output_midi_file = argv[5];
  
  std::cout << "Generate midi file from asco and xml" << std::endl;
  guidohttpd::makeApplication(argc, argv);
  guidohttpd::startEngine();

  MyMapCollector* collector = get_map_collector_from_xml_and_asco(musicxml_file, asco_file, part_filter);
  collector->transpo -= transpo;
  generate_midi(collector, output_midi_file);
  return 0;
}
