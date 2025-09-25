#ifndef GOP_CACHE_HPP
#define GOP_CACHE_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // 屏蔽 Windows 旧版冗余头文件（包括 winsock.h）
#endif
#include "media_packet.hpp"
#include "utils/logger.hpp"
#include <list>

namespace cpp_streamer
{
    class GopCache
    {
    public:
        GopCache(Logger* logger, uint32_t min_gop = 1);
        ~GopCache();

        size_t InsertPacket(Media_Packet_Ptr pkt_ptr);
        int WriterGop(AvWriterInterface* writer_p);

    private:
        Logger* logger_ = nullptr;

    private:
        std::list<Media_Packet_Ptr> packet_list;
        Media_Packet_Ptr video_hdr_;
        Media_Packet_Ptr audio_hdr_;
        Media_Packet_Ptr metadata_hdr_;
        uint32_t min_gop_ = 1;
        uint32_t gop_count_ = 0;
    };
}
#endif