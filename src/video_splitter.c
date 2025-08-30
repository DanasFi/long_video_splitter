#include "video_splitter.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_LAST_CHUNK (45.0 * 60.0)   // 45 minutes in seconds

int cut_video_segment(const char* input_filename, const char* output_filename, 
                     double start_time, double duration) {
    
    AVFormatContext *input_ctx = NULL, *output_ctx = NULL;
    int64_t start_pts = AV_NOPTS_VALUE;
    
    int ret = avformat_open_input(&input_ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input file '%s'\n", input_filename);
        return ret;
    }
    
    ret = avformat_find_stream_info(input_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to retrieve input stream information\n");
        goto cleanup;
    }
    
    ret = avformat_alloc_output_context2(&output_ctx, NULL, NULL, output_filename);
    if (!output_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto cleanup;
    }
    
    for (int i = 0; i < input_ctx->nb_streams; i++) {
        AVStream *input_stream = input_ctx->streams[i];
        AVStream *output_stream = avformat_new_stream(output_ctx, NULL);
        if (!output_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto cleanup;
        }
        
        ret = avcodec_parameters_copy(output_stream->codecpar, input_stream->codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto cleanup;
        }
        output_stream->codecpar->codec_tag = 0; //tells ffmpeg to not force a specific tag
    }
    
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_ctx->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'\n", output_filename);
            goto cleanup;
        }
    }
    
    ret = avformat_write_header(output_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when writing header\n");
        goto cleanup;
    }
    
    int64_t seek_target_time = (int64_t)(start_time * AV_TIME_BASE);
    ret = avformat_seek_file(input_ctx, -1, INT64_MIN, seek_target_time, seek_target_time, 0);
    if (ret < 0) {
        fprintf(stderr, "Error seeking to start time\n");
        goto cleanup;
    }
    
    int64_t end_time_us = (int64_t)((start_time + duration) * AV_TIME_BASE);
    
    AVPacket* packet = av_packet_alloc();
    while (av_read_frame(input_ctx, packet) >= 0) {
        AVStream *input_stream = input_ctx->streams[packet->stream_index];
        AVStream *output_stream = output_ctx->streams[packet->stream_index];
        
        int64_t packet_time_us = av_rescale_q(packet->pts, input_stream->time_base, 
                                              (AVRational){1, AV_TIME_BASE});
        
        if (packet_time_us > end_time_us) {
            av_packet_unref(packet);
            break;
        }
        
        if (packet_time_us < seek_target_time) {
            av_packet_unref(packet);
            continue;
        }
        
        if (start_pts == AV_NOPTS_VALUE && packet->stream_index == 0) {
            start_pts = packet->pts;
        }
        
        packet->pts = av_rescale_q_rnd(packet->pts - start_pts, 
                                      input_stream->time_base, 
                                      output_stream->time_base, 
                                      AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet->dts = av_rescale_q_rnd(packet->dts - start_pts, 
                                      input_stream->time_base, 
                                      output_stream->time_base, 
                                      AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet->duration = av_rescale_q(packet->duration, 
                                       input_stream->time_base, 
                                       output_stream->time_base);
        packet->pos = -1;
        
        ret = av_interleaved_write_frame(output_ctx, packet);
        if (ret < 0) {
            fprintf(stderr, "Error writing packet\n");
            av_packet_unref(packet);
            break;
        }
        
        av_packet_unref(packet);
    }
    
    av_write_trailer(output_ctx);
    
cleanup:
    if (input_ctx) {
        avformat_close_input(&input_ctx);
    }
    if (output_ctx) {
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&output_ctx->pb);
        }
        avformat_free_context(output_ctx);
    }
    
    return ret;
}

double get_video_duration(const char* filename) {
    AVFormatContext *fmt_ctx = NULL;
    double duration = 0.0;
    
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        return -1.0;
    }
    
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        avformat_close_input(&fmt_ctx);
        return -1.0;
    }
    
    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        duration = (double)fmt_ctx->duration / AV_TIME_BASE;
    }
    
    avformat_close_input(&fmt_ctx);
    return duration;
}

void generate_output_filename(const char* input_filename, int part_number, 
                             char* output_filename, size_t max_len) {
    const char* dot = strrchr(input_filename, '.');
    if (dot) {
        size_t name_len = dot - input_filename;
        snprintf(output_filename, max_len, "%.*s_part%02d%s", 
                 (int)name_len, input_filename, part_number, dot);
    } else {
        snprintf(output_filename, max_len, "%s_part%02d", input_filename, part_number);
    }
}

int split_video(const char* input_filename, double chunk_max_duration, double chunk_min_duration) {
    double total_duration = get_video_duration(input_filename);
    if (total_duration <= 0) {
        fprintf(stderr, "Could not get video duration\n");
        return -1;
    }
    
    printf("Total video duration: %.2f seconds (%.2f hours)\n", 
           total_duration, total_duration / 3600.0);
    
    // Calculate number of full chunks and remaining time
    int full_chunks = (int)(total_duration / chunk_max_duration);
    double remaining_time = total_duration - (full_chunks * chunk_max_duration);
    
    printf("Will create %d full chunks of 4 hours each\n", full_chunks);
    printf("Remaining time: %.2f seconds (%.2f minutes)\n", 
           remaining_time, remaining_time / 60.0);
    
    int total_parts;
    double last_chunk_duration;
    int merge_last_chunk = 0;  
    if (remaining_time > 0 && remaining_time < chunk_min_duration && full_chunks > 0) {
        // Merge with previous chunk
        total_parts = full_chunks;
        last_chunk_duration = chunk_max_duration + remaining_time;
        merge_last_chunk = 1;  
        printf("Last segment is too short (%.2f min), merging with previous chunk\n", 
               remaining_time / 60.0);
        printf("Final chunk will be %.2f hours long\n", last_chunk_duration / 3600.0);
    } else {
        // Keep as separate chunk
        total_parts = full_chunks + (remaining_time > 0 ? 1 : 0);
        last_chunk_duration = remaining_time;
        if (remaining_time > 0) {
            printf("Last segment will be %.2f minutes long\n", remaining_time / 60.0);
        }
    }
    
    // Create each part
    for (int i = 0; i < total_parts; i++) {
        char output_filename[512];
        generate_output_filename(input_filename, i + 1, output_filename, sizeof(output_filename));
    
        double start_time = i * chunk_max_duration;
        double duration;
    
        if (i == total_parts - 1 && merge_last_chunk) {
            duration = last_chunk_duration;  // chunk duration + reaming time
        } else if (i == total_parts - 1) {
            duration = last_chunk_duration;  // remaining_time
        } else {
            // Regular chunk
            duration = chunk_max_duration;
        }        
        printf("\nCreating part %d: %s\n", i + 1, output_filename);
        printf("Start time: %.2f seconds (%.2f hours)\n", start_time, start_time / 3600.0);
        printf("Duration: %.2f seconds (%.2f hours)\n", duration, duration / 3600.0);
        
        int ret = cut_video_segment(input_filename, output_filename, start_time, duration);
        if (ret < 0) {
            fprintf(stderr, "Error creating part %d\n", i + 1);
            return ret;
        }
        
        printf("Part %d completed successfully\n", i + 1);
    }
    
    return 0;
}
