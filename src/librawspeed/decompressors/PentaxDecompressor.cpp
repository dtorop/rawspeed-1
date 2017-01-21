#include "common/StdAfx.h"
#include "decompressors/PentaxDecompressor.h"
#include "io/BitPumpMSB.h"
#include "decompressors/HuffmanTable.h"

/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2009-2014 Klaus Post

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA


*/

namespace RawSpeed {

static const uchar8 pentax_tree[] =  {
  0, 2, 3, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0,
//0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5 = 16 entries of codes per bit length
  3, 4, 2, 5, 1, 6, 0, 7, 8, 9, 10, 11, 12
//0  1  2  3  4  5  6  7  8  9  10  11  12       = 13 entries of code values
};

void decodePentax(RawImage& mRaw, ByteStream&& data, TiffIFD* root) {

  HuffmanTable ht;

  /* Attempt to read huffman table, if found in makernote */
  if (root->hasEntryRecursive((TiffTag)0x220)) {
    TiffEntry *t = root->getEntryRecursive((TiffTag)0x220);
    if (t->type == TIFF_UNDEFINED) {

      ByteStream stream = t->getData();

      uint32 depth = (stream.getShort()+12)&0xf;
      stream.skipBytes(12);
      uint32 v0[16];
      uint32 v1[16];
      uint32 v2[16];
      for (uint32 i = 0; i < depth; i++)
         v0[i] = stream.getShort();

      for (uint32 i = 0; i < depth; i++)
        v1[i] = stream.getByte();

      ht.nCodesPerLength.resize(17);

      /* Calculate codes and store bitcounts */
      for (uint32 c = 0; c < depth; c++) {
        v2[c] = v0[c]>>(12-v1[c]);
        ht.nCodesPerLength.at(v1[c])++;
      }
      /* Find smallest */
      for (uint32 i = 0; i < depth; i++) {
        uint32 sm_val = 0xfffffff;
        uint32 sm_num = 0xff;
        for (uint32 j = 0; j < depth; j++) {
          if(v2[j]<=sm_val) {
            sm_num = j;
            sm_val = v2[j];
          }
        }
        ht.codeValues.push_back(sm_num);
        v2[sm_num]=0xffffffff;
      }
    } else {
      ThrowRDE("PentaxDecompressor: Unknown Huffman table type.");
    }
  } else {
    /* Initialize with legacy data */
    auto nCodes = ht.setNCodesPerLength(Buffer(pentax_tree, 16));
    assert(nCodes == 13); // see pentax_tree definition
    ht.setCodeValues(Buffer(pentax_tree+16, nCodes));
  }

  ht.setup(true, false);

  BitPumpMSB bs(data);
  uchar8 *draw = mRaw->getData();
  ushort16 *dest;
  uint32 w = mRaw->dim.x;
  uint32 h = mRaw->dim.y;
  int pUp1[2] = {0, 0};
  int pUp2[2] = {0, 0};
  int pLeft1 = 0;
  int pLeft2 = 0;

  for (uint32 y = 0;y < h;y++) {
    bs.checkPos();
    dest = (ushort16*) & draw[y*mRaw->pitch];  // Adjust destination
    pUp1[y&1] += ht.decodeNext(bs);
    pUp2[y&1] += ht.decodeNext(bs);
    dest[0] = pLeft1 = pUp1[y&1];
    dest[1] = pLeft2 = pUp2[y&1];
    for (uint32 x = 2; x < w ; x += 2) {
      pLeft1 += ht.decodeNext(bs);
      pLeft2 += ht.decodeNext(bs);
      dest[x] =  pLeft1;
      dest[x+1] =  pLeft2;
      _ASSERTE(pLeft1 >= 0 && pLeft1 <= (65536));
      _ASSERTE(pLeft2 >= 0 && pLeft2 <= (65536));
    }
  }
}

} // namespace RawSpeed