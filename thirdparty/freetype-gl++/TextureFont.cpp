/* ===========================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * ----------------------------------------------------------------------------
 * Copyright 2011,2012 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ============================================================================
 */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cwchar>
#include <iostream>

#include "TextureFont.h"
#include "TextureAtlas.h"
#include "FTLibrary.h"

#define HRES  64
#define HRESf 64.f
#define DPI   72

TextureGlyph::TextureGlyph(FT_Face face, wchar_t charcode) :
  face(face), charcode(charcode) {
  id        = 0;
  width     = 0;
  height    = 0;
  outline_type = 0;
  outline_thickness = 0.0;
  offset_x  = 0;
  offset_y  = 0;
  advance_x = 0.0;
  advance_y = 0.0;
  s0        = 0.0;
  t0        = 0.0;
  s1        = 0.0;
  t1        = 0.0;
  if (charcode) {
    glyph_index = FT_Get_Char_Index(face, charcode);
  }
}

float TextureGlyph::getKerning(const wchar_t prevCharcode) {
  if (kerning.find(prevCharcode) == kerning.end()) {
    FT_UInt prev_index;
    FT_Vector kern;
    prev_index = FT_Get_Char_Index(face, prevCharcode);
    FT_Get_Kerning(face, prev_index, glyph_index, FT_KERNING_UNFITTED, &kern);
    kerning[prevCharcode] = kern.x / (HRESf*HRESf);
  }
  return kerning[prevCharcode];
}

TextureFont::TextureFont(TextureAtlas *atlas, const float pointSize, const std::string &filename) :
  _atlas(atlas), _size(pointSize), _filename(filename), _face(NULL) {

  FT_Size_Metrics metrics;

  _height = 0;
  _ascender = 0;
  _descender = 0;
  _outline_type = 0;
  _outline_thickness = 0.0;
  _hinting = true;

  /* Get font metrics at high resolution */
  if (loadFace(_size*100.0)) {

    metrics = _face->size->metrics;
    _ascender = (metrics.ascender >> 6) / 100.0;
    _descender = (metrics.descender >> 6) / 100.0;
    _height = (metrics.height >> 6) / 100.0;

    loadFace(_size);
  }

}

// ---------------------------------------------------- texture_font_delete ---
TextureFont::~TextureFont() {
  if (_face) {
    FT_Done_Face(_face);
  }
}

// ------------------------------------------------- texture_font_load_face ---
bool TextureFont::loadFace(float size) {

  FT_Error error;
  FT_Matrix matrix = {
    static_cast<int>((1.0/HRES) * 0x10000L),
    static_cast<int>((0.0)      * 0x10000L),
    static_cast<int>((0.0)      * 0x10000L),
    static_cast<int>((1.0)      * 0x10000L)};

  assert(size);

  /* Initialize library */
  if(!FTLibrary::instance().isInit()) {
    return false;
  }

  if (_face) {
    FT_Done_Face(_face);
    _face = NULL;
  }

  /* Load face */
  error = FT_New_Face(*FTLibrary::instance().getLibrary(), _filename.c_str(), 0, &_face);

  if(error) {
    FTLibrary::instance().printError(error);
    return false;
  }

  /* Select charmap */
  error = FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
  if(error) {
    FTLibrary::instance().printError(error);
    FT_Done_Face(_face);
    _face = NULL;
    return false;
  }

  /* Set char size */
  error = FT_Set_Char_Size(_face, static_cast<int>(size * HRES), 0, DPI * HRES, DPI);

  if(error) {
    FTLibrary::instance().printError(error);
    FT_Done_Face(_face);
    _face = NULL;
    return false;
  }

  /* Set transform matrix */
  FT_Set_Transform(_face, &matrix, NULL);

  return true;
}

// ----------------------------------------------- texture_font_load_glyphs ---

size_t TextureFont::loadGlyphs(const wchar_t * charcodes) {


  FT_Library library = *FTLibrary::instance().getLibrary();

  size_t missed = 0;

  assert(charcodes);

  size_t width  = _atlas->width();
  size_t height = _atlas->height();

  if (!_face) {
    return wcslen(charcodes);
  }

  /* Load each glyph */
  for(size_t i = 0; i < wcslen(charcodes) ; ++i) {
    TLP_HASH_MAP<wchar_t, TextureGlyph>::iterator it = _glyphs.find(charcodes[i]);
    /* Check if charcode has been already loaded */
    if (it != _glyphs.end()) {
      TextureGlyph *glyph = &it->second;
      if ((glyph->outline_type == _outline_type) &&
          (glyph->outline_thickness == _outline_thickness)) {
        continue;
      }
    }

    FT_Int32 flags = 0;
    int ft_glyph_top = 0;
    int ft_glyph_left = 0;
    FT_UInt glyph_index = FT_Get_Char_Index(_face, charcodes[i]);

    if(_outline_type > 0) {
      flags |= FT_LOAD_NO_BITMAP;
    } else {
      flags |= FT_LOAD_RENDER;
    }

    if(!_hinting) {
      flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
    } else {
      flags |= FT_LOAD_FORCE_AUTOHINT;
    }

    FT_Error error = FT_Load_Glyph(_face, glyph_index, flags);
    if(error) {
      FTLibrary::instance().printError(error);
      return wcslen(charcodes)-i;
    }

    FT_GlyphSlot slot;
    FT_Bitmap ft_bitmap;
    FT_Glyph ft_glyph;

    if(_outline_type == 0) {
      slot            = _face->glyph;
      ft_bitmap       = slot->bitmap;
      ft_glyph_top    = slot->bitmap_top;
      ft_glyph_left   = slot->bitmap_left;
    } else {
      FT_Stroker stroker;
      FT_BitmapGlyph ft_bitmap_glyph;
      error = FT_Stroker_New(library, &stroker);
      if(error) {
        FTLibrary::instance().printError(error);
        FT_Stroker_Done(stroker);
        return wcslen(charcodes)-i;
      }
      FT_Stroker_Set(stroker,
                     static_cast<int>(_outline_thickness * HRES),
                     FT_STROKER_LINECAP_ROUND,
                     FT_STROKER_LINEJOIN_ROUND,
                     0);

      error = FT_Get_Glyph(_face->glyph, &ft_glyph);
      if(error) {
        FTLibrary::instance().printError(error);
        FT_Stroker_Done(stroker);
        return wcslen(charcodes)-i;
      }

      if(_outline_type == 1) {
        error = FT_Glyph_Stroke(&ft_glyph, stroker, 1);
      } else if (_outline_type == 2) {
        error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 0, 1);
      } else if ( _outline_type == 3 ) {
        error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 1, 1);
      }

      if(error) {
        FTLibrary::instance().printError(error);
        FT_Stroker_Done(stroker);
        return wcslen(charcodes)-i;
      }

      error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
      if(error) {
        FTLibrary::instance().printError(error);
        FT_Stroker_Done(stroker);
        return wcslen(charcodes)-i;
      }

      ft_bitmap_glyph = reinterpret_cast<FT_BitmapGlyph>(ft_glyph);
      ft_bitmap       = ft_bitmap_glyph->bitmap;
      ft_glyph_top    = ft_bitmap_glyph->top;
      ft_glyph_left   = ft_bitmap_glyph->left;
      FT_Stroker_Done(stroker);
    }


    // We want each glyph to be separated by at least one black pixel
    // (for example for shader used in demo-subpixel.c)
    size_t w = ft_bitmap.width + 1;
    size_t h = ft_bitmap.rows + 1;
    ivec4 region = _atlas->getRegion(w, h);
    if (region.x < 0) {
      missed++;
      std::cerr << "Texture atlas is full" << std::endl;
      continue;
    }
    w = w - 1;
    h = h - 1;
    size_t x = region.x;
    size_t y = region.y;
    _atlas->setRegion(x, y, w, h, ft_bitmap.buffer, ft_bitmap.pitch);

    TextureGlyph glyph(_face, charcodes[i]);
    glyph.width    = w;
    glyph.height   = h;
    glyph.outline_type = _outline_type;
    glyph.outline_thickness = _outline_thickness;
    glyph.offset_x = ft_glyph_left;
    glyph.offset_y = ft_glyph_top;
    glyph.s0       = x/static_cast<float>(width);
    glyph.t0       = y/static_cast<float>(height);
    glyph.s1       = (x + glyph.width)/static_cast<float>(width);
    glyph.t1       = (y + glyph.height)/static_cast<float>(height);

    // Discard hinting to get advance
    FT_Load_Glyph( _face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
    slot = _face->glyph;
    glyph.advance_x = slot->advance.x / HRESf;
    glyph.advance_y = slot->advance.y / HRESf;

    _glyphs[charcodes[i]] = glyph;

    if( _outline_type > 0 ) {
      FT_Done_Glyph(ft_glyph);
    }
  }

  return missed;
}


// ------------------------------------------------- texture_font_get_glyph ---
TextureGlyph *
TextureFont::getGlyph(wchar_t charcode) {

  TLP_HASH_MAP<wchar_t, TextureGlyph>::iterator it = _glyphs.find(charcode);

  if (it != _glyphs.end()) {
    TextureGlyph *glyph = &(it->second);
    if ((glyph->outline_type == _outline_type) &&
        (glyph->outline_thickness == _outline_thickness)) {
      return glyph;
    } else {
      _glyphs.erase(it);
    }
  }

  /* Glyph has not been already loaded */
  wchar_t buffer[2] = {0,0};
  buffer[0] = charcode;
  if(loadGlyphs(buffer) == 0) {
    return &_glyphs[charcode];
  }
  return NULL;
}
