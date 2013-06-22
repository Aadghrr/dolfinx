// Copyright (C) 2013 Benjamin Kehlet
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// First added:  2013-06-22
// Last changed: 2013-06-22

#include "CSGCGALDomain2D.h"
#include "CSGPrimitives2D.h"
#include "CSGOperators.h"
#include <dolfin/common/constants.h>

#include <CGAL/basic.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Boolean_set_operations_2.h>

// #include <CGAL/Min_circle_2.h>
// #include <CGAL/Min_circle_2_traits_2.h>

// Min enclosing circle typedefs
// typedef CGAL::Min_circle_2_traits_2<Extended_kernel>  Min_Circle_Traits;
// typedef CGAL::Min_circle_2<Min_Circle_Traits>      Min_circle;
// typedef CGAL::Circle_2<Extended_kernel> CGAL_Circle;

typedef CGAL::Exact_predicates_exact_constructions_kernel Exact_Kernel;
typedef Exact_Kernel::Point_2                             Point_2;
typedef CGAL::Polygon_2<Exact_Kernel>                     Polygon_2;
typedef CGAL::Polygon_with_holes_2<Exact_Kernel>          Polygon_with_holes_2;

using namespace dolfin;

struct CSGCGALDomain2DImpl
{
  std::list<Polygon_with_holes_2> polygon_list;
};

Polygon_2 make_circle(const Circle* c)
{
  std::vector<Point_2> pts;

  for (std::size_t i = 0; i < c->fragments(); i++)
  {
    const double phi = (2*DOLFIN_PI*i) / c->fragments();
    const double x = c->center().x() + c->radius()*cos(phi);
    const double y = c->center().y() + c->radius()*sin(phi);
    pts.push_back(Point_2(x, y));
  }

  return Polygon_2(pts.begin(), pts.end());
}
//-----------------------------------------------------------------------------
Polygon_2 make_ellipse(const Ellipse* e)
{
  std::vector<Point_2> pts;

  for (std::size_t i = 0; i < e->fragments(); i++)
  {
    const double phi = (2*DOLFIN_PI*i) / e->fragments();
    const double x = e->center().x() + e->a()*cos(phi);
    const double y = e->center().y() + e->b()*sin(phi);
    pts.push_back(Point_2(x, y));
  }

  return Polygon_2(pts.begin(), pts.end());
}
//-----------------------------------------------------------------------------
Polygon_2 make_rectangle(const Rectangle* r)
{
  // const double x0 = std::min(r->first_corner().x(), r->first_corner().y());
  // const double y0 = std::max(r->first_corner().x(), r->first_corner().y());

  // const double x1 = std::min(r->second_corner().x(), r->second_corner().y());
  // const double y1 = std::max(r->second_corner().x(), r->second_corner().y());

  const double x0 = std::min(r->first_corner().x(), r->second_corner().x());
  const double y0 = std::min(r->first_corner().y(), r->second_corner().y());

  const double x1 = std::max(r->first_corner().x(), r->second_corner().x());
  const double y1 = std::max(r->first_corner().y(), r->second_corner().y());

  std::vector<Point_2> pts;
  pts.push_back(Point_2(x0, y0));
  pts.push_back(Point_2(x1, y0));
  pts.push_back(Point_2(x1, y1));
  pts.push_back(Point_2(x0, y1));

  Polygon_2 p(pts.begin(), pts.end());
  
  return p;
}
//-----------------------------------------------------------------------------
Polygon_2 make_polygon(const Polygon* p)
{
  std::vector<Point_2> pts;
  std::vector<Point>::const_iterator v;
  for (v = p->vertices().begin(); v != p->vertices().end(); ++v)
    pts.push_back(Point_2(v->x(), v->y()));

  return Polygon_2(pts.begin(), pts.end());
}
//-----------------------------------------------------------------------------
CSGCGALDomain2D::CSGCGALDomain2D()
{
  impl = new CSGCGALDomain2DImpl();
}
//-----------------------------------------------------------------------------
CSGCGALDomain2D::~CSGCGALDomain2D()
{
  delete impl;
}
//-----------------------------------------------------------------------------
CSGCGALDomain2D::CSGCGALDomain2D(const CSGGeometry *geometry)
{
  impl = new CSGCGALDomain2DImpl();

  switch (geometry->getType()) 
  {
    case CSGGeometry::Union:
    {
      const CSGUnion *u = dynamic_cast<const CSGUnion*>(geometry);
      dolfin_assert(u);

      // TODO: Optimize this to avoid copying
      CSGCGALDomain2D a(u->_g0.get());
      CSGCGALDomain2D b(u->_g1.get());
    
      *this = a.join(b);
      break;
    }
    case CSGGeometry::Intersection:
    {
      const CSGIntersection* u = dynamic_cast<const CSGIntersection*>(geometry);
      dolfin_assert(u);

      CSGCGALDomain2D a(u->_g0.get());
      CSGCGALDomain2D b(u->_g1.get());
      
      *this = a.intersect(b);
      break;
    }
    case CSGGeometry::Difference:
    {
      const CSGDifference* u = dynamic_cast<const CSGDifference*>(geometry);
      dolfin_assert(u);
      CSGCGALDomain2D a(u->_g0.get());
      CSGCGALDomain2D b(u->_g1.get());
      
      *this = a.difference(b);
      break;
    }
    case CSGGeometry::Circle:
    {
      const Circle* c = dynamic_cast<const Circle*>(geometry);
      dolfin_assert(c);
      impl->polygon_list.push_back(Polygon_with_holes_2(make_circle(c)));
      break;
    }
    case CSGGeometry::Ellipse:
    {
      const Ellipse* c = dynamic_cast<const Ellipse*>(geometry);
      dolfin_assert(c);
      impl->polygon_list.push_back(Polygon_with_holes_2(make_ellipse(c)));
      break;
    }
    case CSGGeometry::Rectangle:
    {
      const Rectangle* r = dynamic_cast<const Rectangle*>(geometry);
      dolfin_assert(r);
      impl->polygon_list.push_back(Polygon_with_holes_2(make_rectangle(r)));
      break;
    }
    case CSGGeometry::Polygon:
    {
      const Polygon* p = dynamic_cast<const Polygon*>(geometry);
      dolfin_assert(p);
      impl->polygon_list.push_back(Polygon_with_holes_2(make_polygon(p)));
      break;
    }
    default:
      dolfin_error("CSGCGALMeshGenerator2D.cpp",
                   "converting geometry to cgal polyhedron",
                   "Unhandled primitive type");
  }
}
//-----------------------------------------------------------------------------
double CSGCGALDomain2D::compute_boundingcircle_radius() const
{
    // // Set the cell size criteria according to the mesh_resolution parameter
    // std::vector<Point_2> points;
    // for (CDT::Point_iterator it = cdt.points_begin();
    //      it != cdt.points_end();
    //      it++)
    //   points.push_back(*it);

    // Min_circle min_circle (points.begin(),
    //                        points.end(),
    //                        true); //randomize point order

  // return min_circle.radius();

  // TODO
  return 1.0;
}
