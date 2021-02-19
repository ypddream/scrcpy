#ifndef VIDEO_BUFFER_H
#define VIDEO_BUFFER_H

#include "common.h"

#include <stdbool.h>

#include "fps_counter.h"
#include "util/thread.h"

// forward declarations
typedef struct AVFrame AVFrame;

/**
 * There are 3 frames in memory:
 *  - one frame is held by the producer (producer_frame)
 *  - one frame is held by the consumer (consumer_frame)
 *  - one frame is shared between the producer and the consumer (pending_frame)
 *
 * The producer generates a frame into the producer_frame (it may takes time).
 *
 * Once the frame is produced, it calls video_buffer_producer_offer_frame(),
 * which swaps the producer and pending frames.
 *
 * When the consumer is notified that a new frame is available, it calls
 * video_buffer_consumer_take_frame() to retrieve it, which swaps the pending
 * and consumer frames. The frame is valid until the next call, without
 * blocking the producer.
 */

struct video_buffer {
    AVFrame *producer_frame;
    AVFrame *pending_frame;
    AVFrame *consumer_frame;

    sc_mutex mutex;
    bool wait_consumer; // never overwrite a pending frame if it is not consumed
    bool interrupted;

    sc_cond pending_frame_consumed_cond;
    bool pending_frame_consumed;

    const struct video_buffer_callbacks *cbs;
    void *cbs_userdata;
};

struct video_buffer_callbacks {
    // Called when a new frame can be consumed by
    // video_buffer_consumer_take_frame(vb)
    void (*on_frame_available)(struct video_buffer *vb, void *userdata);
};

bool
video_buffer_init(struct video_buffer *vb, bool wait_consumer,
                  const struct video_buffer_callbacks *cbs, void *cbs_userdata);

void
video_buffer_destroy(struct video_buffer *vb);

// set the producer frame as ready for consuming
// the output flag is set to report whether the previous frame has been skipped
void
video_buffer_producer_offer_frame(struct video_buffer *vb,
                                  bool *previous_frame_skipped);

// mark the consumer frame as consumed and return it
// the frame is valid until the next call to this function
const AVFrame *
video_buffer_consumer_take_frame(struct video_buffer *vb);

// wake up and avoid any blocking call
void
video_buffer_interrupt(struct video_buffer *vb);

#endif
