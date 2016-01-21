/**
 *
 * This file is part of Tulip (www.tulip-software.org)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux 1 and Inria Bordeaux - Sud Ouest
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#ifndef CURVEBUILDER_H
#define CURVEBUILDER_H

#include <tulip/PropertyAlgorithm.h>

class CurveBuilder:public tlp::LayoutAlgorithm {
public:
  
  PLUGININFORMATIONS("Curve Builder", "Ludwig Fiolka", "07/01/2011", "Statically generates bezier curve points from an existing graph layout and allows the rendering to be made in polylines", "0.1", "")
  
  CurveBuilder(tlp::PluginContext* context);
  virtual ~CurveBuilder();
  
  bool run();
  bool check(std::string &);
};

#endif // CURVEBUILDER_H



