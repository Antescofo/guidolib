#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cerr << "Usage: "
         << argv[0]
         << " audio_raw(Float 48000Hz) duration(in sec) output_best_extract_audio"
         << endl;
    return 1;
  }
  std::string input_file = argv[1];
  float duration_sec = std::stof(argv[2]);
  std::string output_file = argv[3];
  float rate = 48000;
  int nframe = round(rate * duration_sec);

  std::cout << "DURATION SEC:" << duration_sec << std::endl;
  std::cout << "NFRAME:" << nframe << std::endl;
  auto pFile = fopen(input_file.c_str(), "rb");
  if (pFile == NULL) {
    std::cerr << "Error reading audio file: does not exist" << std::endl;
    return 1;
  }
  fseek(pFile, 0, SEEK_END);
  auto sz = ftell(pFile);
  fseek(pFile, 0, SEEK_SET);

  long total_frame = sz / sizeof(float);
  float* buffer = new float[sz / sizeof(float)];
  size_t res = fread(buffer, 1, sz, pFile);
  if (res != sz) {
    std::cerr << "Error reading audio file" << std::endl;
    return 1;
  }

  double max_energy = 0;
  long max_offset = 0;
  double local_energy = 0;
  long offset_start = 0;
  for (long k = 0; k < nframe; ++k) {
    local_energy += buffer[k];
  }
  for (long k = nframe + 1; k < total_frame; ++k) {
    local_energy -= buffer[offset_start];
    local_energy += buffer[k];
    if (local_energy > max_energy) {
      max_energy = local_energy;
      max_offset = offset_start;
    }
    ++offset_start;
  }
  std::cout << "TOTAL FILE FRAME:" << total_frame << std::endl;

  std::cout << "MAX ENERGY:" << max_energy << std::endl;
  std::cout << "MAX OFFSET:" << max_offset << std::endl;

  fclose(pFile);
  pFile = fopen(output_file.c_str(), "wb");
  fwrite(buffer + max_offset, 1, nframe * sizeof(float), pFile);
  fclose(pFile);

  delete[] buffer;
  return 0;
}
