#include "gop_cache.hpp"
#include "logger.hpp"

namespace cpp_streamer
{
	GopCache::GopCache(Logger* logger, uint32_t min_gop) : logger_(logger)
        , min_gop_(min_gop) {

    }

    GopCache::~GopCache() {

    }

    size_t GopCache::InsertPacket(Media_Packet_Ptr pkt_ptr) {
        if (pkt_ptr->av_type_ == MEDIA_VIDEO_TYPE) {
            if (pkt_ptr->is_seq_hdr_) {
                video_hdr_ = pkt_ptr;
                return packet_list.size();
            }
            if (pkt_ptr->is_key_frame_) {
                gop_count_++;
                if ((gop_count_ % min_gop_) == 0) {
                    packet_list.clear();
                }
            }
        }
        else if (pkt_ptr->av_type_ == MEDIA_AUDIO_TYPE) {
            if (pkt_ptr->is_seq_hdr_) {
                audio_hdr_ = pkt_ptr;
                return packet_list.size();
            }
        }
        else if (pkt_ptr->av_type_ == MEDIA_METADATA_TYPE) {
            metadata_hdr_ = pkt_ptr;
            LogInfof(logger_, "update rtmp metadata len:%lu", metadata_hdr_->buffer_ptr_->DataLen());
            return packet_list.size();
        }
        else {
            LogWarnf(logger_, "unkown av type:%d", pkt_ptr->av_type_);
            return 0;
        }

        packet_list.push_back(pkt_ptr);
        return packet_list.size();
    }

    int GopCache::WriterGop(AvWriterInterface* writer_p) {
        int ret = 0;

        if (metadata_hdr_.get() && metadata_hdr_->buffer_ptr_->DataLen() > 0) {
            //log_info_data((uint8_t*)metadata_hdr_->buffer_ptr_->data(),
            //        metadata_hdr_->buffer_ptr_->data_len(), "metadata hdr");
            ret = writer_p->WritePacket(metadata_hdr_);
            if (ret < 0) {
                return ret;
            }
        }

        if (video_hdr_.get() && video_hdr_->buffer_ptr_->DataLen() > 0) {
            //log_info_data((uint8_t*)video_hdr_->buffer_ptr_->data(),
            //        video_hdr_->buffer_ptr_->data_len(), "video hdr data");
            ret = writer_p->WritePacket(video_hdr_);
            if (ret < 0) {
                return ret;
            }
        }

        if (audio_hdr_.get() && audio_hdr_->buffer_ptr_->DataLen() > 0) {
            //log_info_data((uint8_t*)audio_hdr_->buffer_ptr_->data(),
            //        audio_hdr_->buffer_ptr_->data_len(), "audio hdr data");
            ret = writer_p->WritePacket(audio_hdr_);
            if (ret < 0) {
                return ret;
            }
        }

        for (auto iter : packet_list) {
            Media_Packet_Ptr pkt_ptr = iter;
            ret = writer_p->WritePacket(pkt_ptr);
            if (ret < 0) {
                return ret;
            }
        }

        return ret;
    }

}