/* ============================================================================
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
 *
 * This source is based on the article by Jukka Jylänki :
 * "A Thousand Ways to Pack the Bin - A Practical Approach to
 * Two-Dimensional Rectangle Bin Packing", February 27, 2010.
 *
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *  ============================================================================
 */
#ifndef __TEXTURE_ATLAS_H__
#define __TEXTURE_ATLAS_H__

#include <cstdlib>
#include <vector>

#include "vec234.h"

/**
 * @file   texture-atlas.h
 * @author Nicolas Rougier (Nicolas.Rougier@inria.fr)
 *
 * @defgroup texture-atlas Texture atlas
 *
 * A texture atlas is used to pack several small regions into a single texture.
 *
 * The actual implementation is based on the article by Jukka Jylänki : "A
 * Thousand Ways to Pack the Bin - A Practical Approach to Two-Dimensional
 * Rectangle Bin Packing", February 27, 2010.
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *
 * Example Usage:
 * @code
 * #include "texture-atlas.h"
 *
 * ...
 *
 * / Creates a new atlas of 512x512 with a depth of 1
 * texture_atlas_t * atlas = texture_atlas_new( 512, 512, 1 );
 *
 * // Allocates a region of 20x20
 * ivec4 region = texture_atlas_get_region( atlas, 20, 20 );
 *
 * // Fill region with some data
 * texture_atlas_set_region( atlas, region.x, region.y, region.width, region.height, data, stride )
 *
 * ...
 *
 * @endcode
 *
 * @{
 */


/**
 * A texture atlas is used to pack several small regions into a single texture.
 */
class TextureAtlas {

public:

  /**
   * Creates a new empty texture atlas.
   *
   * @param   width   width of the atlas
   * @param   height  height of the atlas
   * @param   depth   bit depth of the atlas
   *
   */
  TextureAtlas(const size_t width, const size_t height, const size_t depth);

  /**
   *  Deletes a texture atlas.
   *
   *
   */
  ~TextureAtlas();

  /**
   *  Upload atlas to video memory.
   *
   *
   */
  void upload();

  bool needUpload() const {return _needUpload;}

  /**
   *  Allocate a new region in the atlas.
   *
   *  @param width  width of the region to allocate
   *  @param height height of the region to allocate
   *  @return       Coordinates of the allocated region
   *
   */
  ivec4 getRegion(const size_t width, const size_t height);

  /**
   *  Upload data to the specified atlas region.
   *
   *  @param x      x coordinate the region
   *  @param y      y coordinate the region
   *  @param width  width of the region
   *  @param height height of the region
   *  @param data   data to be uploaded into the specified region
   *  @param stride stride of the data
   *
   */
  void setRegion(const size_t x,
                 const size_t y,
                 const size_t width,
                 const size_t height,
                 const unsigned char *data,
                 const size_t stride);
  /**
   *  Remove all allocated regions from the atlas.
   *
   */
  void clear();

  void bind();

  size_t width() {return _width;}

  size_t height() {return _height;}

  size_t depth() {return _depth;}

  size_t used() {return _used;}

  unsigned int id() {return _id;}

  unsigned char *data() {return _data;}

private:

  int fit(const size_t index, const size_t width, const size_t height );

  void merge();

  /**
     * Allocated nodes
     */
  std::vector<ivec3> _nodes;

  /**
     *  Width (in pixels) of the underlying texture
     */
  size_t _width;

  /**
     * Height (in pixels) of the underlying texture
     */
  size_t _height;

  /**
     * Depth (in bytes) of the underlying texture
     */
  size_t _depth;

  /**
     * Allocated surface size
     */
  size_t _used;

  /**
     * Texture identity (OpenGL)
     */
  unsigned int _id;

  /**
     * Atlas data
     */
  unsigned char * _data;

  bool _needUpload;

};

#endif /* __TEXTURE_ATLAS_H__ */
