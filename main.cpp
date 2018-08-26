// ============================================================================
//
// This sandbox is partially based on the following Theora example:
//  https://github.com/xiph/theora/blob/master/examples/player_example.c
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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const auto BUFFER_SIZE = 4096;

enum class State { STOPPED, STARTED };

static ogg_stream_state to;

static th_setup_info* ts = nullptr;


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

  ogg_page page;
  auto state = State::STOPPED;
  while (state == State::STOPPED) {
    // ========================================================================
    // READ DATA
    // here we read data from the source file into the OGG parser. data is read
    // and handled one OGG buffer at a time. see readData for more details.
    // ========================================================================
    auto bytes = readData(*file, oss);
    if (bytes == 0) {
      break;
    }

    while (ogg_sync_pageout(&oss, &page) > 0) {
      // ======================================================================
      // DETECT THE END OF THE HEADER SECTION
      // check whether the iteration has found the end of the OGG file headers.
      // ======================================================================
      if (ogg_page_bos(&page) <= 0) {
        state = State::STARTED;
        break;
      }

      // ======================================================================
      // CREATE AND READ A OGG PACKET FROM A STREAM
      // create a OGG stream from the target OGG page and provide a OGG packet
      // from the currently iterated OGG page.
      // ======================================================================
      auto result = 0;
      ogg_packet packet;
      ogg_stream_state test;
      result = ogg_stream_init(&test, ogg_page_serialno(&page));
      assert(result == 0);
      result = ogg_stream_pagein(&test, &page);
      assert(result == 0);
      result = ogg_stream_packetout(&test, &packet);
      assert(result == 1);

      // ======================================================================
      // IDENTIFY THE CODEC
      // identify what kind of content the target OGG contains. here te most
      // common types would be to check whether we have Theora and/or Vorbis.
      // ======================================================================
      if (th_decode_headerin(&ti, &tc, &ts, &packet) >= 0) {
        printf("the provided test.ogg contains Theora video data.\n");
        memcpy(&to, &test, sizeof(test));
      } else {
        ogg_stream_clear(&test);
      }
    }
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
