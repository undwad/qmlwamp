#pragma once

#include <QObject>
#include <QString>
#include <QDataStream>
#include <QByteArray>
#include <QDebug>
#include <QtZlib/zlib.h>

#define HEAD_CRC 0x02
#define EXTRA_FIELD 0x04
#define ORIG_NAME 0x08
#define COMMENT 0x10
#define RESERVED 0xE0
#define CHUNK 16384

class GUnzip
{
public:
    static void decompress(QByteArray &compressed, QByteArray &inflated)
    {
        GUnzip().gunzipBodyPartially(compressed, inflated);
    }

private:
    bool initInflate = false;
    bool streamEnd = false;
    z_stream inflateStrm;

    GUnzip() { }

    bool gzipCheckHeader(QByteArray &content, int &pos)
    {
        const unsigned char gz_magic[2] = {0x1f, 0x8b};

        int method = 0;
        int flags = 0;
        bool ret = false;

        pos = -1;
        QByteArray &body = content;
        int maxPos = body.size()-1;
        if (maxPos < 1) return ret;

        if (body[0] != char(gz_magic[0]) || body[1] != char(gz_magic[1])) return ret;

        pos += 2;
        if (++pos <= maxPos) method = body[pos];
        if (pos++ <= maxPos) flags = body[pos];
        if (method != Z_DEFLATED || (flags & RESERVED) != 0) return ret;

        pos += 6;
        if (pos > maxPos) return ret;

        if ((flags & EXTRA_FIELD) && ((pos+2) <= maxPos))
        {
            unsigned len =  (unsigned)body[++pos];
            len += ((unsigned)body[++pos])<<8;
            pos += len;
            if (pos > maxPos) return ret;
        }

        if ((flags & ORIG_NAME) != 0) while(++pos <= maxPos && body[pos]) ;

        if ((flags & COMMENT) != 0) while(++pos <= maxPos && body[pos]) ;

        if ((flags & HEAD_CRC) != 0)
        {
            pos += 2;
            if (pos > maxPos) return ret;
        }

        ret = (pos < maxPos);

        return ret;
    }

    int gunzipBodyPartially(QByteArray &compressed, QByteArray &inflated)
    {
        int ret = Z_DATA_ERROR;
        unsigned have;
        unsigned char out[CHUNK];
        int pos = -1;

        if (!initInflate)
        {
            //if (!gzipCheckHeader(compressed, pos)) return ret;

            inflateStrm.zalloc = Z_NULL;
            inflateStrm.zfree = Z_NULL;
            inflateStrm.opaque = Z_NULL;
            inflateStrm.avail_in = 0;
            inflateStrm.next_in = Z_NULL;

            ret = inflateInit2(&inflateStrm, -MAX_WBITS);
            if (ret != Z_OK) return ret;

            initInflate = true;
            streamEnd = false;
        }

        compressed.remove(0, pos+1);
        inflateStrm.next_in = (unsigned char *)compressed.data();
        inflateStrm.avail_in = compressed.size();
        do
        {
            inflateStrm.avail_out = sizeof(out);
            inflateStrm.next_out = out;
            ret = inflate(&inflateStrm, Z_NO_FLUSH);
            switch (ret)
            {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&inflateStrm);
                initInflate = false;
                return ret;
            }
            have = sizeof(out) - inflateStrm.avail_out;
            inflated.append(QByteArray((const char *)out, have));
         }
        while (inflateStrm.avail_out == 0 && inflateStrm.avail_in > 0);

        if (ret <= Z_ERRNO || ret == Z_STREAM_END) gunzipBodyPartiallyEnd();
        streamEnd = (ret == Z_STREAM_END);
        return ret;
    }

    void gunzipBodyPartiallyEnd()
    {
        if (initInflate)
        {
            inflateEnd(&inflateStrm);
            initInflate = false;
        }
    }
};

#undef HEAD_CRC
#undef EXTRA_FIELD
#undef ORIG_NAME
#undef COMMENT
#undef RESERVED
#undef CHUNK

