// ============================================================================
//
// Some additional notes about using Theora:
//
// 1. Encoded Theora frames must be a multiple of 16 in size. The info header
//    attributes th_info.frame_width and th_info.frame_height present this kind
//    of values. Arbitary picture sizes are specified in the th_info.pic_x,
//    th_info.pic_y, th_info.pic_width and th_info.pic_height attributes.
//
// 2. It is generally recommended that the offsets and sizes are multiples of
//    2 to avoid chroma sampling shifts if chroma is sub-sampled.
//
// ============================================================================
#include <ogg/ogg.h>
#include <theora/theoradec.h>

#include <cstdio>
#include <cstdlib>

const auto BUFFER_SIZE = 4096;

enum class State { STOPPED, STARTED };

// ==========================================================================
// A helper function to read data block from a file into the OGG container.
// @param file The file to read from.
// @param state The OGG synchronization state.
// ==========================================================================
static int readData(FILE& file, ogg_sync_state& state) {
  auto buffer = ogg_sync_buffer(&state, BUFFER_SIZE);
  auto bytes = fread(buffer, 1, BUFFER_SIZE, &file);
  if (ogg_sync_wrote(&state, bytes) != 0) {
    printf("ogg_sync_wrote failed: An internal error occured.\n");
    exit(EXIT_FAILURE);
  }
  return bytes;
}

// ==========================================================================

int main()
{
  printf("%s\n", th_version_string());

  // ==========================================================================
  // OPEN THE TARGET OGG FILE
  // open the target OGG file so we can start parsing it with our OGG parser.
  // ==========================================================================
  FILE* file = fopen("test.ogg", "rb");
  if (file == nullptr) {
    printf("fopen failed: Unable to locate test.ogg file.\n");
    exit(EXIT_FAILURE);
  }

  // init OGG sync state for data retrieval.
  ogg_sync_state oss;
  ogg_sync_init(&oss);

  // ==========================================================================
  // INITIALIZE COMMENT INFORMATION
  // initialize in-stream metadata container for the comment header packet.
  // this header section should be short (not more than a short paragraph).
  // ==========================================================================
  th_comment tc;
  th_comment_init(&tc);

  // ==========================================================================
  // INITIALIZE BITSTREAM INFORMATION
  // initialize bitstream information container for the info header packet.
  // this header section contains the main information about the video file.
  // ==========================================================================
  th_info ti;
  th_info_init(&ti);

  auto state = State::STOPPED;
  while (state == State::STOPPED) {

  }

  // free the memory allocated for the header containers.
  th_info_clear(&ti);
  th_comment_clear(&tc);

  // free the OGG sync storage and reset to initial state.
  ogg_sync_clear(&oss);

  // close the OGG file.
  fclose(file);
  return EXIT_SUCCESS;
}
