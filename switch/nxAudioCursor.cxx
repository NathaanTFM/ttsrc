#include "nxAudioCursor.h"
#include <opus/opusfile.h>

namespace libnx {
    #include <switch.h>
}

#include "virtualFileSystem.h"

TypeHandle NxAudioCursor::_type_handle;

// This is from libnx example ; we consider we should be working
// in a same thread, so let's hope this is safe enough
static size_t opuspkt_tmpbuf_size = sizeof(libnx::HwopusHeader) + 4096*48;
static libnx::u8* opuspkt_tmpbuf;

int hw_decode(void *_ctx, OpusMSDecoder *_decoder, void *_pcm, const ogg_packet *_op, int _nsamples, int _nchannels, int _format, int _li) {
    printf("attempting hw decode\n");
    
    libnx::HwopusDecoder *decoder = (libnx::HwopusDecoder*)_ctx;
    libnx::HwopusHeader *hdr = NULL;
    size_t pktsize, pktsize_extra;

    libnx::Result rc = 0;
    libnx::s32 DecodedDataSize = 0;
    libnx::s32 DecodedSampleCount = 0;

    printf("check format\n");
    if (_format != OP_DEC_FORMAT_SHORT) return OPUS_BAD_ARG;

    pktsize = _op->bytes;//Opus packet size.
    pktsize_extra = pktsize+8;//Packet size with HwopusHeader.

    printf("check buf size\n");
    if (pktsize_extra > opuspkt_tmpbuf_size) return OPUS_INTERNAL_ERROR;

    printf("do stuff\n");
    hdr = (libnx::HwopusHeader*)opuspkt_tmpbuf;
    memset(opuspkt_tmpbuf, 0, pktsize_extra);

    hdr->size = __builtin_bswap32(pktsize);
    printf("set mem stuff\n");
    memcpy(&opuspkt_tmpbuf[sizeof(libnx::HwopusHeader)], _op->packet, pktsize);

    printf("!! attempt decode\n");
    rc = libnx::hwopusDecodeInterleaved(decoder, &DecodedDataSize, &DecodedSampleCount, opuspkt_tmpbuf, pktsize_extra, (libnx::s16*)_pcm, _nsamples * _nchannels * sizeof(opus_int16));

    printf("did it work?\n");
    if (R_FAILED(rc)) return OPUS_INTERNAL_ERROR;
    
    printf("is it correct?\n");
    if (DecodedDataSize != pktsize_extra || DecodedSampleCount != _nsamples) return OPUS_INVALID_PACKET;

    printf("nice!\n");
    return 0;
}

int custom_read(istream *_stream, char *_ptr, int _nbytes)
{
    printf("custom_read()\n");
    _stream->clear();
    _stream->read(_ptr, _nbytes);
    if (_stream->fail())
    {
        return -1;
    }
    printf("nice\n");
    return _stream->gcount();
}

int custom_seek(istream *_stream, opus_int64 _offset, int _whence)
{
    printf("custom_seek\n");
    std::ios_base::seekdir way = std::ios_base::beg;
    if (_whence == SEEK_SET)
        way = std::ios_base::beg;
    else if (_whence == SEEK_CUR)
        way = std::ios_base::cur;
    else if (_whence == SEEK_END)
        way = std::ios_base::end;
    
    _stream->clear();
    _stream->seekg(_offset, way);
    if (_stream->fail())
    {
        return -1;
    }
    printf("nice\n");
    return 0;
}

opus_int64 custom_tell(istream *_stream)
{
    printf("custom_tell - nice?\n");
    return _stream->tellg();
}



NxAudioCursor::
NxAudioCursor(NxAudio *src) :
    MovieAudioCursor(src)
{
    _opus_file = NULL;
    
    static libnx::HwopusDecoder hwdecoder = {0};
    static bool initialized = false;
    
    if (!initialized)
    {
        opuspkt_tmpbuf = (libnx::u8*)malloc(opuspkt_tmpbuf_size);
    
        libnx::Result res = libnx::hwopusDecoderInitialize(&hwdecoder, 48000, 2);
        printf("hwopus init %d\n", res);
        initialized = true;
    }
    
    VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
    istream *strm = vfs->open_read_file(Filename::binary_filename(src->_filename), true);
    
    if (!strm)
    {
        printf("strm failed.\n");
        return;
    }
    
    const OpusFileCallbacks cb = {
        .read = (op_read_func)custom_read,
        .seek = (op_seek_func)custom_seek,
        .tell = (op_tell_func)custom_tell
        //.op_close_func = strm->close
    };
    
    int error;
    //_opus_file = op_open_file(src->_filename.to_os_specific().c_str(), &error);
    _opus_file = op_open_callbacks(strm, &cb, NULL, 0, &error);
    
    printf("opus open file! %d\n", error);
    if (!_opus_file)
        return;
    
    op_set_decode_callback(_opus_file, hw_decode, &hwdecoder);
}

NxAudioCursor::
~NxAudioCursor()
{
    op_free(_opus_file);
}

void NxAudioCursor::
seek(double t)
{
    printf("seek %f\n", t);
    //ogg_int64_t target_ts = (ogg_int64_t)(t / unkyetTODO);
    
    //int opret = op_pcm_seek(_opus_file, ogg_int64_t);
    //printf("seek rep %d\n", opret);
}

void NxAudioCursor::
read_samples(int n, PN_int16 *data)
{
    printf("hi we got here! (%d samples)\n", n);
    if (!_opus_file)
        return;
    
    int samples_read = 0;
    int give_up_after = 100;
    
    while (n > samples_read && give_up_after > 0)
    {
        give_up_after--;
        
        int opret = op_read(_opus_file, data + samples_read, (n - samples_read), NULL);
        printf("opret %d\n", opret);
        if (opret <= 0)
            break;
        
        samples_read += opret;
        printf("samples_read %d\n", samples_read);
    }
}