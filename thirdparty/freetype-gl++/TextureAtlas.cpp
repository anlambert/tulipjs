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
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <climits>
#include <iostream>

#include <GL/glew.h>

#include <edtaa3func.h>

#include "TextureAtlas.h"

// make_distance_map function from demo-distance-field.c in freetype-gl
static unsigned char *
make_distance_map(unsigned char *img,
                  unsigned int width, unsigned int height) {

  short *xdist = new short[width * height];
  short *ydist = new short[width * height];
  double *gx  = new double[width * height]();
  double *gy  = new double[width * height]();
  double *data = new double[width * height]();
  double *outside = new double[width * height]();
  double *inside = new double[width * height]();
  unsigned int i;

  // Convert img into double (data)
  double img_min = 255, img_max = -255;
  for (i = 0 ; i < width * height ; ++i) {
    double v = img[i];
    data[i] = v;
    if (v > img_max) img_max = v;
    if (v < img_min) img_min = v;
  }

  // Rescale image levels between 0 and 1
  for (i=0; i < width * height ; ++i) {
    data[i] = (img[i]-img_min)/img_max;
  }

  // Compute outside = edtaa3(bitmap); % Transform background (0's)
  computegradient(data, width, height, gx, gy);
  edtaa3(data, gx, gy, width, height, xdist, ydist, outside);
  for (i = 0; i < width * height ; ++i)
    if( outside[i] < 0)
      outside[i] = 0.0;

  // Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
  memset(gx, 0, sizeof(double)*width*height);
  memset(gy, 0, sizeof(double)*width*height);
  for (i = 0 ; i < width*height ; ++i)
    data[i] = 1 - data[i];
  computegradient(data, width, height, gx, gy);
  edtaa3(data, gx, gy, width, height, xdist, ydist, inside);
  for (i = 0 ; i < width * height ; ++i)
    if(inside[i] < 0)
      inside[i] = 0.0;

  // distmap = outside - inside; % Bipolar distance field
  unsigned char *out = new unsigned char[width * height];
  for (i = 0 ; i < width * height ; ++i) {
    outside[i] -= inside[i];
    outside[i] = 128+outside[i]*16;
    if( outside[i] < 0 ) outside[i] = 0;
    if( outside[i] > 255 ) outside[i] = 255;
    out[i] = 255 - static_cast<unsigned char>(outside[i]);
    //out[i] = (unsigned char) outside[i];
  }

  delete [] xdist;
  delete [] ydist;
  delete [] gx;
  delete [] gy;
  delete [] data;
  delete [] outside;
  delete [] inside;
  return out;
}

TextureAtlas::TextureAtlas( const size_t width, const size_t height, const size_t depth, bool useDistanceField) :
  _width(width), _height(height), _depth(depth), _used(0), _id(0), _data(NULL), _needUpload(false), _useDistanceField(useDistanceField) {

  assert((depth == 1) || (depth == 3) || (depth == 4));

  // We want a one pixel border around the whole atlas to avoid any artefact when
  // sampling texture
  ivec3 node(1,1,width-2);
  _nodes.push_back(node);

  _data = new unsigned char[width*height*depth]();
}


// --------------------------------------------------- texture_atlas_delete ---
TextureAtlas::~TextureAtlas() {
  delete [] _data;
  if(_id) {
    glDeleteTextures( 1, &_id );
  }
}


// ----------------------------------------------- texture_atlas_set_region ---
void
TextureAtlas::setRegion(const size_t x,
                        const size_t y,
                        const size_t width,
                        const size_t height,
                        const unsigned char * data,
                        const size_t stride) {
  assert(x > 0);
  assert(y > 0);
  assert(x < (_width-1));
  assert((x + width) <= (_width-1));
  assert(y < (_height-1));
  assert((y + height) <= (_height-1));

  size_t charsize = sizeof(char);
  for(size_t i = 0 ; i < height ; ++i) {
    memcpy(_data+((y+i)*_width + x ) * charsize * _depth,
           data + (i*stride) * charsize, width * charsize * _depth);
  }
  _needUpload = true;
}


// ------------------------------------------------------ texture_atlas_fit ---
int
TextureAtlas::fit(const size_t index,
                  const size_t width,
                  const size_t height) {

  ivec3 *node = &_nodes[index];
  int x = node->x;
  int y = node->y;
  int width_left = width;
  size_t i = index;

  if ((x + width) > (_width-1)) {
    return -1;
  }
  y = node->y;
  while(width_left > 0) {
    node = &_nodes[i];
    if(node->y > y) {
      y = node->y;
    }
    if((y + height) > (_height-1)) {
      return -1;
    }
    width_left -= node->z;
    ++i;
  }
  return y;
}


// ---------------------------------------------------- texture_atlas_merge ---
void
TextureAtlas::merge() {
  ivec3 *node, *next;

  for(size_t i = 0 ; i < _nodes.size() - 1 ; ++i) {
    node = &_nodes[i];
    next = &_nodes[i+1];
    if(node->y == next->y) {
      node->z += next->z;
      _nodes.erase(_nodes.begin() + i+1);
      --i;
    }
  }
}


// ----------------------------------------------- texture_atlas_get_region ---
ivec4
TextureAtlas::getRegion(const size_t width,
                        const size_t height) {

  ivec3 *node, *prev;
  ivec4 region(0,0,width,height);

  int best_height = INT_MAX;
  int best_index  = -1;
  int best_width = INT_MAX;
  for(size_t i = 0 ; i < _nodes.size() ; ++i) {
    int y = fit(i, width, height);
    if(y >= 0) {
      node = &_nodes[i];
      if(((y + int(height)) < best_height) ||
         (((y + int(height)) == best_height) && (node->z < best_width))) {
        best_height = y + height;
        best_index = i;
        best_width = node->z;
        region.x = node->x;
        region.y = y;
      }
    }
  }

  if(best_index == -1) {
    region.x = -1;
    region.y = -1;
    region.z = 0;
    region.w = 0;
    return region;
  }

  node = new ivec3();

  node->x = region.x;
  node->y = region.y + height;
  node->z = width;
  _nodes.insert(_nodes.begin()+best_index, *node);

  delete node;

  for(size_t i = best_index+1 ; i < _nodes.size() ; ++i) {
    node = &_nodes[i];
    prev = &_nodes[i-1];

    if (node->x < (prev->x + prev->z)) {
      int shrink = prev->x + prev->z - node->x;
      node->x += shrink;
      node->z -= shrink;
      if (node->z <= 0) {
        _nodes.erase(_nodes.begin()+i);
        --i;
      } else {
        break;
      }
    } else {
      break;
    }
  }
  merge();
  _used += width * height;
  return region;
}


// ---------------------------------------------------- texture_atlas_clear ---
void
TextureAtlas::clear() {
  ivec3 node(1,1,1);

  assert( _data );

  _nodes.clear();
  _used = 0;
  // We want a one pixel border around the whole atlas to avoid any artefact when
  // sampling texture
  node.z = _width-2;

  _nodes.push_back(node);
  memset( _data, 0, _width*_height*_depth );
}


// --------------------------------------------------- texture_atlas_upload ---
void
TextureAtlas::upload() {
  assert( _data );

  if (!_needUpload) {
    return;
  }

  if( !_id ) {
    glGenTextures( 1, &_id );
  }

  unsigned char *data = _data;
  if (_useDistanceField) {
    data = make_distance_map(_data, _width, _height);
  }

  glBindTexture(GL_TEXTURE_2D, _id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  if (_useDistanceField) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }

  if(_depth == 4) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  } else if(_depth == 3) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _width, _height,
                 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
  }

  if (_useDistanceField) {
    delete [] data;
  } else {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  _needUpload = false;
}

void TextureAtlas::bind() {
  if (_needUpload) {
    upload();
  }
  glBindTexture(GL_TEXTURE_2D, _id);
}

