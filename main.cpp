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
#define SDL_MAIN_HANDLED

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <SDL2/SDL.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const auto BUFFER_SIZE = 4096;

enum class State { STOPPED, STARTED };

// ==========================================================================

static ogg_stream_state to;

static th_dec_ctx* td = nullptr;
static th_setup_info* ts = nullptr;
static int th_header_count = 0;

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

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
// A helper function to add a complete page to OGG bitstream.
// @param page The page to be added.
// ==========================================================================
static void queuePage(ogg_page& page) {
  if (th_header_count > 0) {
    if (ogg_stream_pagein(&to, &page) != 0) {
      // printf("ogg_stream_pagein failed: internal failure occured.\n");
      // exit(EXIT_FAILURE);
    }
  }
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
  ogg_packet packet;
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
        queuePage(page);
        state = State::STARTED;
        break;
      }

      // ======================================================================
      // CREATE AND READ A OGG PACKET FROM A STREAM
      // create a OGG stream from the target OGG page and provide a OGG packet
      // from the currently iterated OGG page.
      // ======================================================================
      auto result = 0;
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
      if (!th_header_count && th_decode_headerin(&ti, &tc, &ts, &packet) >= 0) {
        printf("the provided test.ogg contains Theora video data.\n");
        memcpy(&to, &test, sizeof(test));
        th_header_count = 1;
      } else {
        ogg_stream_clear(&test);
      }
    }

    // ========================================================================
    // PARSE ALL HEADERS
    // identify and parse rest of the headers from the provided OGG packet.
    // ========================================================================
    while (th_header_count && th_header_count < 3) {
      auto result = ogg_stream_packetout(&to, &packet);
      if (result < 0) {
        printf("ogg_stream_packetout failed: failed to get steam packets.\n");
        exit(EXIT_FAILURE);
      } else if (!th_decode_headerin(&ti, &tc, &ts, &packet)) {
        printf("ogg_stream_packetout failed: failed to get Theora headers.\n");
        exit(EXIT_FAILURE);
      }
      th_header_count++;

      // ======================================================================
      // DEMUX PAGES TO STREAM
      // insert pages into corresponding streams.
      // ======================================================================
      if (ogg_sync_pageout(&oss, &page) > 0) {
        queuePage(page);
      } else {
        bytes = readData(*file, oss);
        if (bytes == 0) {
          printf("readData failed: EOF while searching code headers.\n");
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  // ==========================================================================
  // ALLOCATE A DECODER INSTANCE
  // allocate and build a decoder instance that can be used in the decode loop.
  // ==========================================================================
  td = th_decode_alloc(&ti, ts);
  printf("OGG stream %lx is Theora %dx%d %.02f fps\n",
    to.serialno, ti.pic_width, ti.pic_height,
    static_cast<double>(ti.fps_numerator/ti.fps_denominator));
  switch (ti.pixel_fmt) {
    case TH_PF_420: printf("  4:2:0 video\n"); break;
    case TH_PF_422: printf("  4:2:2 video\n"); break;
    case TH_PF_444: printf("  4:4:4 video\n"); break;
    default:
      printf("\tvideo is UNKNOWN Chroma sampling\n");
      break;
  }
  if (ti.pic_width != ti.frame_width || ti.pic_height != ti.frame_height) {
    printf("  frame is %dx%d with offset %d,%d\n",
      ti.frame_width, ti.frame_height, ti.pic_x, ti.pic_y);
  }

  // ==========================================================================
  // DETECT AND SET THE POST-PROCESSING LEVEL
  // define the level of post-processing to be used with the decode funcions.
  // here we query the maximum post-processing level and assign it to decoder.
  // ==========================================================================
  int pp = 0;
  int maxPp = 0;
  int ppInc = 0;
  th_decode_ctl(td, TH_DECCTL_GET_PPLEVEL_MAX, &maxPp, sizeof(maxPp));
  printf("maximum post-processing level: %d\n", maxPp);
  pp = maxPp;
  th_decode_ctl(td, TH_DECCTL_SET_PPLEVEL, &pp, sizeof(pp));
  ppInc = 0;

  // release storage used for the decoder setup.
  th_setup_free(ts);

  // ==========================================================================
  // DETECT RENDERING AREA SIZE
  // calculate the real dimensions required for our rendering system to render
  // the video data on the screen. remember the section 1 in "additional notes"
  // at the start of this file
  // ==========================================================================
  auto width = (ti.pic_x + ti.pic_width + 1 &~ 1) - (ti.pic_x &~ 1);
  auto height = (ti.pic_y + ti.pic_height + 1 &~ 1) - (ti.pic_y &~ 1);

  // ==========================================================================
  // INIT VIDEO SYSTEM
  // initialize a video system so we can present our images to the screen. here
  // we use SDL as an example, but this could be done with other API:s as well.
  // ==========================================================================
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // INIT VIDEO SYSTEM WINDOW
  // construct a window for the video system so we have a surface to draw.
  // ==========================================================================
  window = SDL_CreateWindow("Video",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    width, height,
    SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // INIT VIDEO SYSTEM RENDERER
  // construct a renderer for the created window to support surface drawing.
  // ==========================================================================
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  // TODO start the main decode loop.

  // release SDL components.
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // free the memory allocated for the OGG stream.
  ogg_stream_clear(&to);

  // free the memory allocated for the decoder context.
  th_decode_free(td);

  // free the memory allocated for the header containers.
  th_info_clear(&ti);
  th_comment_clear(&tc);

  // free the OGG sync storage and reset to initial state.
  ogg_sync_clear(&oss);

  // close the OGG file.
  fclose(file);
  return EXIT_SUCCESS;
}
