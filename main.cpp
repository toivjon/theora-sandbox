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

int main()
{
  printf("%s\n", th_version_string());

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



  // free the memory allocated for the header containers.
  th_info_clear(&ti);
  th_comment_clear(&tc);

  // free the OGG sync storage and reset to initial state.
  ogg_sync_clear(&oss);
  return EXIT_SUCCESS;
}
