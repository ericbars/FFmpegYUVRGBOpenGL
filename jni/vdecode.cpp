#include "player.h"


void* video_thread(void *argv) {
	AVPacket pkt1;
	AVPacket *packet = &pkt1;
	int frameFinished;
	AVFrame *pFrame;
	double pts;

	EGLBoolean success = eglMakeCurrent(global_context.eglDisplay,
			global_context.eglSurface, global_context.eglSurface,
			global_context.eglContext);

	CreateProgram();

	for (;;) {

		if (global_context.quit) {
			av_log(NULL, AV_LOG_ERROR, "video_thread need exit. \n");
			break;
		}

		if (global_context.pause) {
			continue;
		}

		if (packet_queue_get(&global_context.video_queue, packet) <= 0) {
			// means we quit getting packets
			continue;
		}

		pFrame = av_frame_alloc();

		avcodec_decode_video2(global_context.vcodec_ctx, pFrame, &frameFinished,
				packet);

		/*av_log(NULL, AV_LOG_ERROR,
		 "packet_queue_get size is %d, format is %d\n", packet->size,
		 pFrame->format);*/

		// Did we get a video frame?
		if (frameFinished) {
			renderSurface(pFrame);
			av_frame_unref(pFrame);
		}

		av_packet_unref(packet);
		av_init_packet(packet);

		// about framerate
		usleep(1000);
	}

	av_free(pFrame);

	return 0;
}

