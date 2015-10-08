/* ============================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * ----------------------------------------------------------------------------
 * Copyright 2011,2012,2013 Nicolas P. Rougier. All rights reserved.
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
#ifndef __VEC234_H__
#define __VEC234_H__


/**
 * Tuple of 4 ints.
 *
 */

struct ivec4 {

  ivec4(int x=0, int y=0, int z=0, int w=0) : x(x), y(y), z(z), w(w) {}

  int x;
  int y;
  int z;
  int w;
};


/**
 * Tuple of 3 ints.
 *
 */

struct ivec3 {

  ivec3(int x=0, int y=0, int z=0) : x(x), y(y), z(z) {}

  int x;
  int y;
  int z;
};



/**
 * Tuple of 2 ints.
 *
 */

struct ivec2 {

  ivec2(int x=0, int y=0) : x(x), y(y) {}

  int x;
  int y;
};


/**
 * Tuple of 4 floats.
 *
 */
struct vec4 {

  vec4(float x=0, float y=0, float z=0, float w=0) : x(x), y(y), z(z), w(w) {}

  float x;
  float y;
  float z;
  float w;
};



/**
 * Tuple of 3 floats
 *
 */

struct vec3 {

  vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}

  float x;
  float y;
  float z;
};



/**
 * Tuple of 2 floats
 *
 */

struct vec2 {

  vec2(float x=0, float y=0) : x(x), y(y) {}

  float x;
  float y;
};

#endif /* __VEC234_H__ */
