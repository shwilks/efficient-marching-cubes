/**
 * @file    MarchingCubes.cpp
 * @author  Thomas Lewiner <thomas.lewiner@polytechnique.org>
 * @author  Math Dept, PUC-Rio
 * @version 0.2
 * @date    12/08/2002
 *
 * @brief   MarchingCubes Algorithm
 */
//________________________________________________


#include <math.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <float.h>
#include <cmath>
#include <limits>
#include "MarchingCubes.h"
#include "ply.h"
#include "LookUpTable.h"

// step size of the arrays of vertices and triangles
#define ALLOC_SIZE 65536

//_____________________________________________________________________________
// print cube for debug
void MarchingCubes::print_cube() { printf( "\t%f %f %f %f %f %f %f %f\n", _cube[0], _cube[1], _cube[2], _cube[3], _cube[4], _cube[5], _cube[6], _cube[7]) ; }
//_____________________________________________________________________________



//_____________________________________________________________________________
// Constructor
MarchingCubes::MarchingCubes( const int size_x /*= -1*/, const int size_y /*= -1*/, const int size_z /*= -1*/ ) :
//-----------------------------------------------------------------------------
  _originalMC(false),
  _size_x    (size_x),
  _size_y    (size_y),
  _size_z    (size_z),
  _nverts    (0),
  _ntrigs    (0),
  _Nverts    (0),
  _Ntrigs    (0)
{}
//_____________________________________________________________________________



//_____________________________________________________________________________
// Destructor
MarchingCubes::~MarchingCubes()
//-----------------------------------------------------------------------------
{
  clean_all() ;
}
//_____________________________________________________________________________



//_____________________________________________________________________________
// main algorithm
void MarchingCubes::run( real iso )
//-----------------------------------------------------------------------------
{
  clock_t time = clock() ;

  compute_intersection_points( iso ) ;

  for( _k = 0 ; _k < _size_z-1 ; _k++ )
  for( _j = 0 ; _j < _size_y-1 ; _j++ )
  for( _i = 0 ; _i < _size_x-1 ; _i++ )
  {
    _lut_entry = 0 ;
    for( int p = 0 ; p < 8 ; ++p )
    {
      _cube[p] = get_data( _i+((p^(p>>1))&1), _j+((p>>1)&1), _k+((p>>2)&1) ) - iso ;
			if( std::abs( _cube[p] ) < std::numeric_limits<float>::epsilon() ) _cube[p] = std::numeric_limits<float>::epsilon() ;
      if( _cube[p] > 0 ) _lut_entry += 1 << p ;
    }

    process_cube( ) ;
  }

  printf("Marching Cubes ran in %lf secs.\n", (double)(clock() - time)/CLOCKS_PER_SEC) ;
}
//_____________________________________________________________________________



//_____________________________________________________________________________
// init temporary structures (must set sizes before call)
void MarchingCubes::init_temps()
//-----------------------------------------------------------------------------
{
	_data.resize(_size_x * _size_y * _size_z);
	_x_verts.resize(_size_x * _size_y * _size_z);
	_y_verts.resize(_size_x * _size_y * _size_z);
	_z_verts.resize(_size_x * _size_y * _size_z);
	
	memset( _x_verts.data(), -1, _x_verts.size() * sizeof( int ) ) ;
  memset( _y_verts.data(), -1, _y_verts.size() * sizeof( int ) ) ;
  memset( _z_verts.data(), -1, _z_verts.size() * sizeof( int ) ) ;
}
//_____________________________________________________________________________



//_____________________________________________________________________________
// init all structures (must set sizes before call)
void MarchingCubes::init_all ()
//-----------------------------------------------------------------------------
{
  init_temps() ;

  _nverts = _ntrigs = 0 ;
  _Nverts = _Ntrigs = ALLOC_SIZE ;
  _vertices.resize(_Nverts);
	_triangles.resize(_Ntrigs);
}
//_____________________________________________________________________________


//_____________________________________________________________________________
// clean all structures
void MarchingCubes::clean_all()
//-----------------------------------------------------------------------------
{
  _nverts = _ntrigs = 0 ;
  _Nverts = _Ntrigs = 0 ;

  _size_x = _size_y = _size_z = -1 ;
}
//_____________________________________________________________________________



//_____________________________________________________________________________
//_____________________________________________________________________________


//_____________________________________________________________________________
// Compute the intersection points
void MarchingCubes::compute_intersection_points( real iso )
//-----------------------------------------------------------------------------
{
  for( _k = 0 ; _k < _size_z ; _k++ )
  for( _j = 0 ; _j < _size_y ; _j++ )
  for( _i = 0 ; _i < _size_x ; _i++ )
  {
    _cube[0] = get_data( _i, _j, _k ) - iso ;
    if( _i < _size_x - 1 ) _cube[1] = get_data(_i+1, _j , _k ) - iso ;
    else                   _cube[1] = _cube[0] ;

    if( _j < _size_y - 1 ) _cube[3] = get_data( _i ,_j+1, _k ) - iso ;
    else                   _cube[3] = _cube[0] ;

    if( _k < _size_z - 1 ) _cube[4] = get_data( _i , _j ,_k+1) - iso ;
    else                   _cube[4] = _cube[0] ;

		if( std::abs( _cube[0] ) < std::numeric_limits<float>::epsilon() ) _cube[0] = std::numeric_limits<float>::epsilon() ;
    if( std::abs( _cube[1] ) < std::numeric_limits<float>::epsilon() ) _cube[1] = std::numeric_limits<float>::epsilon() ;
    if( std::abs( _cube[3] ) < std::numeric_limits<float>::epsilon() ) _cube[3] = std::numeric_limits<float>::epsilon() ;
    if( std::abs( _cube[4] ) < std::numeric_limits<float>::epsilon() ) _cube[4] = std::numeric_limits<float>::epsilon() ;

    if( _cube[0] < 0 )
    {
      if( _cube[1] > 0 ) set_x_vert( add_x_vertex( ), _i,_j,_k ) ;
      if( _cube[3] > 0 ) set_y_vert( add_y_vertex( ), _i,_j,_k ) ;
      if( _cube[4] > 0 ) set_z_vert( add_z_vertex( ), _i,_j,_k ) ;
    }
    else
    {
      if( _cube[1] < 0 ) set_x_vert( add_x_vertex( ), _i,_j,_k ) ;
      if( _cube[3] < 0 ) set_y_vert( add_y_vertex( ), _i,_j,_k ) ;
      if( _cube[4] < 0 ) set_z_vert( add_z_vertex( ), _i,_j,_k ) ;
    }
  }
}
//_____________________________________________________________________________





//_____________________________________________________________________________
// Test a face
// if face>0 return true if the face contains a part of the surface
bool MarchingCubes::test_face( schar face )
//-----------------------------------------------------------------------------
{
  static int corner_lookup[6][4] = {
		{0, 4, 5, 1},
		{1, 5, 6, 2},
		{2, 6, 7, 3},
		{3, 7, 4, 0},
		{0, 3, 2, 1},
		{4, 7, 6, 5}
	};
	
	auto idx = std::abs(face) - 1;
	auto corners = corner_lookup[idx];
	auto A = _cube[corners[0]];
	auto B = _cube[corners[1]];
	auto C = _cube[corners[2]];
	auto D = _cube[corners[3]];

	if(std::abs(A * C - B * D) < std::numeric_limits<float>::epsilon()) {
    return face >= 0 ;
	}
	
	// face and A invert signs
  return face * A * ( A * C - B * D ) >= 0.f ;
}
//_____________________________________________________________________________





//_____________________________________________________________________________
// Test the interior of a cube
// if s == 7, return true  if the interior is empty
// if s ==-7, return false if the interior is empty
bool MarchingCubes::test_interior( schar s )
//-----------------------------------------------------------------------------
{
  real t, At=0, Bt=0, Ct=0, Dt=0, a, b ;
  char  test =  0 ;
  char  edge = -1 ; // reference edge of the triangulation

  switch( _case )
  {
  case  4 :
  case 10 :
    a = ( _cube[4] - _cube[0] ) * ( _cube[6] - _cube[2] ) - ( _cube[7] - _cube[3] ) * ( _cube[5] - _cube[1] ) ;
    b =  _cube[2] * ( _cube[4] - _cube[0] ) + _cube[0] * ( _cube[6] - _cube[2] )
             - _cube[1] * ( _cube[7] - _cube[3] ) - _cube[3] * ( _cube[5] - _cube[1] ) ;
    t = - b / (2*a) ;
    if( t<0 || t>1 ) return s>0 ;

    At = _cube[0] + ( _cube[4] - _cube[0] ) * t ;
    Bt = _cube[3] + ( _cube[7] - _cube[3] ) * t ;
    Ct = _cube[2] + ( _cube[6] - _cube[2] ) * t ;
    Dt = _cube[1] + ( _cube[5] - _cube[1] ) * t ;
    break ;

  case  6 :
  case  7 :
  case 12 :
  case 13 :
    switch( _case )
    {
    case  6 : edge = test6 [_config][2] ; break ;
    case  7 : edge = test7 [_config][4] ; break ;
    case 12 : edge = test12[_config][3] ; break ;
    case 13 : edge = tiling13_5_1[_config][_subconfig][0] ; break ;
    }
    switch( edge )
    {
    case  0 :
      t  = _cube[0] / ( _cube[0] - _cube[1] ) ;
      At = 0 ;
      Bt = _cube[3] + ( _cube[2] - _cube[3] ) * t ;
      Ct = _cube[7] + ( _cube[6] - _cube[7] ) * t ;
      Dt = _cube[4] + ( _cube[5] - _cube[4] ) * t ;
      break ;
    case  1 :
      t  = _cube[1] / ( _cube[1] - _cube[2] ) ;
      At = 0 ;
      Bt = _cube[0] + ( _cube[3] - _cube[0] ) * t ;
      Ct = _cube[4] + ( _cube[7] - _cube[4] ) * t ;
      Dt = _cube[5] + ( _cube[6] - _cube[5] ) * t ;
      break ;
    case  2 :
      t  = _cube[2] / ( _cube[2] - _cube[3] ) ;
      At = 0 ;
      Bt = _cube[1] + ( _cube[0] - _cube[1] ) * t ;
      Ct = _cube[5] + ( _cube[4] - _cube[5] ) * t ;
      Dt = _cube[6] + ( _cube[7] - _cube[6] ) * t ;
      break ;
    case  3 :
      t  = _cube[3] / ( _cube[3] - _cube[0] ) ;
      At = 0 ;
      Bt = _cube[2] + ( _cube[1] - _cube[2] ) * t ;
      Ct = _cube[6] + ( _cube[5] - _cube[6] ) * t ;
      Dt = _cube[7] + ( _cube[4] - _cube[7] ) * t ;
      break ;
    case  4 :
      t  = _cube[4] / ( _cube[4] - _cube[5] ) ;
      At = 0 ;
      Bt = _cube[7] + ( _cube[6] - _cube[7] ) * t ;
      Ct = _cube[3] + ( _cube[2] - _cube[3] ) * t ;
      Dt = _cube[0] + ( _cube[1] - _cube[0] ) * t ;
      break ;
    case  5 :
      t  = _cube[5] / ( _cube[5] - _cube[6] ) ;
      At = 0 ;
      Bt = _cube[4] + ( _cube[7] - _cube[4] ) * t ;
      Ct = _cube[0] + ( _cube[3] - _cube[0] ) * t ;
      Dt = _cube[1] + ( _cube[2] - _cube[1] ) * t ;
      break ;
    case  6 :
      t  = _cube[6] / ( _cube[6] - _cube[7] ) ;
      At = 0 ;
      Bt = _cube[5] + ( _cube[4] - _cube[5] ) * t ;
      Ct = _cube[1] + ( _cube[0] - _cube[1] ) * t ;
      Dt = _cube[2] + ( _cube[3] - _cube[2] ) * t ;
      break ;
    case  7 :
      t  = _cube[7] / ( _cube[7] - _cube[4] ) ;
      At = 0 ;
      Bt = _cube[6] + ( _cube[5] - _cube[6] ) * t ;
      Ct = _cube[2] + ( _cube[1] - _cube[2] ) * t ;
      Dt = _cube[3] + ( _cube[0] - _cube[3] ) * t ;
      break ;
    case  8 :
      t  = _cube[0] / ( _cube[0] - _cube[4] ) ;
      At = 0 ;
      Bt = _cube[3] + ( _cube[7] - _cube[3] ) * t ;
      Ct = _cube[2] + ( _cube[6] - _cube[2] ) * t ;
      Dt = _cube[1] + ( _cube[5] - _cube[1] ) * t ;
      break ;
    case  9 :
      t  = _cube[1] / ( _cube[1] - _cube[5] ) ;
      At = 0 ;
      Bt = _cube[0] + ( _cube[4] - _cube[0] ) * t ;
      Ct = _cube[3] + ( _cube[7] - _cube[3] ) * t ;
      Dt = _cube[2] + ( _cube[6] - _cube[2] ) * t ;
      break ;
    case 10 :
      t  = _cube[2] / ( _cube[2] - _cube[6] ) ;
      At = 0 ;
      Bt = _cube[1] + ( _cube[5] - _cube[1] ) * t ;
      Ct = _cube[0] + ( _cube[4] - _cube[0] ) * t ;
      Dt = _cube[3] + ( _cube[7] - _cube[3] ) * t ;
      break ;
    case 11 :
      t  = _cube[3] / ( _cube[3] - _cube[7] ) ;
      At = 0 ;
      Bt = _cube[2] + ( _cube[6] - _cube[2] ) * t ;
      Ct = _cube[1] + ( _cube[5] - _cube[1] ) * t ;
      Dt = _cube[0] + ( _cube[4] - _cube[0] ) * t ;
      break ;
    default : printf( "Invalid edge %d\n", edge ) ;  print_cube() ;  break ;
    }
    break ;

  default : printf( "Invalid ambiguous case %d\n", _case ) ;  print_cube() ;  break ;
  }

  if( At >= 0 ) test ++ ;
  if( Bt >= 0 ) test += 2 ;
  if( Ct >= 0 ) test += 4 ;
  if( Dt >= 0 ) test += 8 ;
  switch( test )
  {
  case  0 : return s>0 ;
  case  1 : return s>0 ;
  case  2 : return s>0 ;
  case  3 : return s>0 ;
  case  4 : return s>0 ;
  case  5 : if( At * Ct - Bt * Dt <  std::numeric_limits<float>::epsilon() ) return s>0 ; break ;
  case  6 : return s>0 ;
  case  7 : return s<0 ;
  case  8 : return s>0 ;
  case  9 : return s>0 ;
  case 10 : if( At * Ct - Bt * Dt >= std::numeric_limits<float>::epsilon() ) return s>0 ; break ;
  case 11 : return s<0 ;
  case 12 : return s>0 ;
  case 13 : return s<0 ;
  case 14 : return s<0 ;
  case 15 : return s<0 ;
  }

  return s<0 ;
}
//_____________________________________________________________________________




//_____________________________________________________________________________
// Process a unit cube
void MarchingCubes::process_cube( )
//-----------------------------------------------------------------------------
{
  if( _originalMC )
  {
    char nt = 0 ;
    while( casesClassic[_lut_entry][3*nt] != -1 ) nt++ ;
    add_triangle( casesClassic[_lut_entry], nt ) ;
    return ;
  }

  int   v12 = -1 ;
  _case   = cases[_lut_entry][0] ;
  _config = cases[_lut_entry][1] ;
  _subconfig = 0 ;

  switch( _case )
  {
  case  0 :
    break ;

  case  1 :
    add_triangle( tiling1[_config], 1 ) ;
    break ;

  case  2 :
    add_triangle( tiling2[_config], 2 ) ;
    break ;

  case  3 :
    if( test_face( test3[_config]) )
      add_triangle( tiling3_2[_config], 4 ) ; // 3.2
    else
      add_triangle( tiling3_1[_config], 2 ) ; // 3.1
    break ;

  case  4 :
    if( test_interior( test4[_config]) )
      add_triangle( tiling4_1[_config], 2 ) ; // 4.1.1
    else
      add_triangle( tiling4_2[_config], 6 ) ; // 4.1.2
    break ;

  case  5 :
    add_triangle( tiling5[_config], 3 ) ;
    break ;

  case  6 :
    if( test_face( test6[_config][0]) )
      add_triangle( tiling6_2[_config], 5 ) ; // 6.2
    else
    {
      if( test_interior( test6[_config][1]) )
        add_triangle( tiling6_1_1[_config], 3 ) ; // 6.1.1
      else
	  {
        v12 = add_c_vertex() ;
        add_triangle( tiling6_1_2[_config], 9 , v12) ; // 6.1.2
      }
    }
    break ;

  case  7 :
    if( test_face( test7[_config][0] ) ) _subconfig +=  1 ;
    if( test_face( test7[_config][1] ) ) _subconfig +=  2 ;
    if( test_face( test7[_config][2] ) ) _subconfig +=  4 ;
    switch( _subconfig )
      {
      case 0 :
        add_triangle( tiling7_1[_config], 3 ) ; break ;
      case 1 :
        add_triangle( tiling7_2[_config][0], 5 ) ; break ;
      case 2 :
        add_triangle( tiling7_2[_config][1], 5 ) ; break ;
      case 3 :
        v12 = add_c_vertex() ;
        add_triangle( tiling7_3[_config][0], 9, v12 ) ; break ;
      case 4 :
        add_triangle( tiling7_2[_config][2], 5 ) ; break ;
      case 5 :
        v12 = add_c_vertex() ;
        add_triangle( tiling7_3[_config][1], 9, v12 ) ; break ;
      case 6 :
        v12 = add_c_vertex() ;
        add_triangle( tiling7_3[_config][2], 9, v12 ) ; break ;
      case 7 :
        if( test_interior( test7[_config][3]) )
          add_triangle( tiling7_4_2[_config], 9 ) ;
        else
          add_triangle( tiling7_4_1[_config], 5 ) ;
        break ;
      };
    break ;

  case  8 :
    add_triangle( tiling8[_config], 2 ) ;
    break ;

  case  9 :
    add_triangle( tiling9[_config], 4 ) ;
    break ;

  case 10 :
    if( test_face( test10[_config][0]) )
    {
      if( test_face( test10[_config][1]) )
        add_triangle( tiling10_1_1_[_config], 4 ) ; // 10.1.1
      else
      {
        v12 = add_c_vertex() ;
        add_triangle( tiling10_2[_config], 8, v12 ) ; // 10.2
      }
    }
    else
    {
      if( test_face( test10[_config][1]) )
      {
        v12 = add_c_vertex() ;
        add_triangle( tiling10_2_[_config], 8, v12 ) ; // 10.2
      }
      else
      {
        if( test_interior( test10[_config][2]) )
          add_triangle( tiling10_1_1[_config], 4 ) ; // 10.1.1
        else
          add_triangle( tiling10_1_2[_config], 8 ) ; // 10.1.2
      }
    }
    break ;

  case 11 :
    add_triangle( tiling11[_config], 4 ) ;
    break ;

  case 12 :
    if( test_face( test12[_config][0]) )
    {
      if( test_face( test12[_config][1]) )
        add_triangle( tiling12_1_1_[_config], 4 ) ; // 12.1.1
      else
      {
        v12 = add_c_vertex() ;
        add_triangle( tiling12_2[_config], 8, v12 ) ; // 12.2
      }
    }
    else
    {
      if( test_face( test12[_config][1]) )
      {
        v12 = add_c_vertex() ;
        add_triangle( tiling12_2_[_config], 8, v12 ) ; // 12.2
      }
      else
      {
        if( test_interior( test12[_config][2]) )
          add_triangle( tiling12_1_1[_config], 4 ) ; // 12.1.1
        else
          add_triangle( tiling12_1_2[_config], 8 ) ; // 12.1.2
      }
    }
    break ;

  case 13 :
    if( test_face( test13[_config][0] ) ) _subconfig +=  1 ;
    if( test_face( test13[_config][1] ) ) _subconfig +=  2 ;
    if( test_face( test13[_config][2] ) ) _subconfig +=  4 ;
    if( test_face( test13[_config][3] ) ) _subconfig +=  8 ;
    if( test_face( test13[_config][4] ) ) _subconfig += 16 ;
    if( test_face( test13[_config][5] ) ) _subconfig += 32 ;
    switch( subconfig13[_subconfig] )
    {
      case 0 :/* 13.1 */
        add_triangle( tiling13_1[_config], 4 ) ; break ;

      case 1 :/* 13.2 */
        add_triangle( tiling13_2[_config][0], 6 ) ; break ;
      case 2 :/* 13.2 */
        add_triangle( tiling13_2[_config][1], 6 ) ; break ;
      case 3 :/* 13.2 */
        add_triangle( tiling13_2[_config][2], 6 ) ; break ;
      case 4 :/* 13.2 */
        add_triangle( tiling13_2[_config][3], 6 ) ; break ;
      case 5 :/* 13.2 */
        add_triangle( tiling13_2[_config][4], 6 ) ; break ;
      case 6 :/* 13.2 */
        add_triangle( tiling13_2[_config][5], 6 ) ; break ;

      case 7 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][0], 10, v12 ) ; break ;
      case 8 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][1], 10, v12 ) ; break ;
      case 9 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][2], 10, v12 ) ; break ;
      case 10 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][3], 10, v12 ) ; break ;
      case 11 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][4], 10, v12 ) ; break ;
      case 12 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][5], 10, v12 ) ; break ;
      case 13 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][6], 10, v12 ) ; break ;
      case 14 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][7], 10, v12 ) ; break ;
      case 15 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][8], 10, v12 ) ; break ;
      case 16 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][9], 10, v12 ) ; break ;
      case 17 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][10], 10, v12 ) ; break ;
      case 18 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3[_config][11], 10, v12 ) ; break ;

      case 19 :/* 13.4 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_4[_config][0], 12, v12 ) ; break ;
      case 20 :/* 13.4 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_4[_config][1], 12, v12 ) ; break ;
      case 21 :/* 13.4 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_4[_config][2], 12, v12 ) ; break ;
      case 22 :/* 13.4 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_4[_config][3], 12, v12 ) ; break ;

      case 23 :/* 13.5 */
        _subconfig = 0 ;
        if( test_interior( test13[_config][6] ) )
          add_triangle( tiling13_5_1[_config][0], 6 ) ;
        else
          add_triangle( tiling13_5_2[_config][0], 10 ) ;
        break ;
      case 24 :/* 13.5 */
        _subconfig = 1 ;
        if( test_interior( test13[_config][6] ) )
          add_triangle( tiling13_5_1[_config][1], 6 ) ;
        else
          add_triangle( tiling13_5_2[_config][1], 10 ) ;
        break ;
      case 25 :/* 13.5 */
        _subconfig = 2 ;
        if( test_interior( test13[_config][6] ) )
          add_triangle( tiling13_5_1[_config][2], 6 ) ;
        else
          add_triangle( tiling13_5_2[_config][2], 10 ) ;
        break ;
      case 26 :/* 13.5 */
        _subconfig = 3 ;
        if( test_interior( test13[_config][6] ) )
          add_triangle( tiling13_5_1[_config][3], 6 ) ;
        else
          add_triangle( tiling13_5_2[_config][3], 10 ) ;
        break ;

      case 27 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][0], 10, v12 ) ; break ;
      case 28 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][1], 10, v12 ) ; break ;
      case 29 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][2], 10, v12 ) ; break ;
      case 30 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][3], 10, v12 ) ; break ;
      case 31 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][4], 10, v12 ) ; break ;
      case 32 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][5], 10, v12 ) ; break ;
      case 33 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][6], 10, v12 ) ; break ;
      case 34 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][7], 10, v12 ) ; break ;
      case 35 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][8], 10, v12 ) ; break ;
      case 36 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][9], 10, v12 ) ; break ;
      case 37 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][10], 10, v12 ) ; break ;
      case 38 :/* 13.3 */
        v12 = add_c_vertex() ;
        add_triangle( tiling13_3_[_config][11], 10, v12 ) ; break ;

      case 39 :/* 13.2 */
        add_triangle( tiling13_2_[_config][0], 6 ) ; break ;
      case 40 :/* 13.2 */
        add_triangle( tiling13_2_[_config][1], 6 ) ; break ;
      case 41 :/* 13.2 */
        add_triangle( tiling13_2_[_config][2], 6 ) ; break ;
      case 42 :/* 13.2 */
        add_triangle( tiling13_2_[_config][3], 6 ) ; break ;
      case 43 :/* 13.2 */
        add_triangle( tiling13_2_[_config][4], 6 ) ; break ;
      case 44 :/* 13.2 */
        add_triangle( tiling13_2_[_config][5], 6 ) ; break ;

      case 45 :/* 13.1 */
        add_triangle( tiling13_1_[_config], 4 ) ; break ;

      default :
        printf("Marching Cubes: Impossible case 13?\n" ) ;  print_cube() ;
      }
      break ;

  case 14 :
    add_triangle( tiling14[_config], 4 ) ;
    break ;
  };
}
//_____________________________________________________________________________



//_____________________________________________________________________________
// Adding triangles
void MarchingCubes::add_triangle( const char* trig, char n, int v12 )
//-----------------------------------------------------------------------------
{
  int    tv[3] ;

  for( int t = 0 ; t < 3*n ; t++ )
  {
    switch( trig[t] )
    {
    case  0 : tv[ t % 3 ] = get_x_vert( _i , _j , _k ) ; break ;
    case  1 : tv[ t % 3 ] = get_y_vert(_i+1, _j , _k ) ; break ;
    case  2 : tv[ t % 3 ] = get_x_vert( _i ,_j+1, _k ) ; break ;
    case  3 : tv[ t % 3 ] = get_y_vert( _i , _j , _k ) ; break ;
    case  4 : tv[ t % 3 ] = get_x_vert( _i , _j ,_k+1) ; break ;
    case  5 : tv[ t % 3 ] = get_y_vert(_i+1, _j ,_k+1) ; break ;
    case  6 : tv[ t % 3 ] = get_x_vert( _i ,_j+1,_k+1) ; break ;
    case  7 : tv[ t % 3 ] = get_y_vert( _i , _j ,_k+1) ; break ;
    case  8 : tv[ t % 3 ] = get_z_vert( _i , _j , _k ) ; break ;
    case  9 : tv[ t % 3 ] = get_z_vert(_i+1, _j , _k ) ; break ;
    case 10 : tv[ t % 3 ] = get_z_vert(_i+1,_j+1, _k ) ; break ;
    case 11 : tv[ t % 3 ] = get_z_vert( _i ,_j+1, _k ) ; break ;
    case 12 : tv[ t % 3 ] = v12 ; break ;
    default : break ;
    }

    if( tv[t%3] == -1 )
    {
      printf("Marching Cubes: invalid triangle %d\n", _ntrigs+1) ;
      print_cube() ;
    }

    if( t%3 == 2 )
    {
      if( _ntrigs >= _Ntrigs )
      {
        _Ntrigs *= 2 ;
				_triangles.resize(_Ntrigs);
      }

      Triangle *T = _triangles.data() + _ntrigs++ ;
      T->v1    = tv[0] ;
      T->v2    = tv[1] ;
      T->v3    = tv[2] ;
    }
  }
}
//_____________________________________________________________________________



//_____________________________________________________________________________
// Calculating gradient

real MarchingCubes::get_x_grad( const int i, const int j, const int k ) const
//-----------------------------------------------------------------------------
{
  if( i > 0 )
  {
    if ( i < _size_x - 1 )
      return ( get_data( i+1, j, k ) - get_data( i-1, j, k ) ) / 2 ;
    else
      return get_data( i, j, k ) - get_data( i-1, j, k ) ;
  }
  else
    return get_data( i+1, j, k ) - get_data( i, j, k ) ;
}
//-----------------------------------------------------------------------------

real MarchingCubes::get_y_grad( const int i, const int j, const int k ) const
//-----------------------------------------------------------------------------
{
  if( j > 0 )
  {
    if ( j < _size_y - 1 )
      return ( get_data( i, j+1, k ) - get_data( i, j-1, k ) ) / 2 ;
    else
      return get_data( i, j, k ) - get_data( i, j-1, k ) ;
  }
  else
    return get_data( i, j+1, k ) - get_data( i, j, k ) ;
}
//-----------------------------------------------------------------------------

real MarchingCubes::get_z_grad( const int i, const int j, const int k ) const
//-----------------------------------------------------------------------------
{
  if( k > 0 )
  {
    if ( k < _size_z - 1 )
      return ( get_data( i, j, k+1 ) - get_data( i, j, k-1 ) ) / 2 ;
    else
      return get_data( i, j, k ) - get_data( i, j, k-1 ) ;
  }
  else
    return get_data( i, j, k+1 ) - get_data( i, j, k ) ;
}
//_____________________________________________________________________________


//_____________________________________________________________________________
// Adding vertices

void MarchingCubes::test_vertex_addition()
{
  if( _nverts >= _Nverts )
  {
    _Nverts *= 2 ;
		_vertices.resize(_Nverts);
  }
}


int MarchingCubes::add_x_vertex( )
//-----------------------------------------------------------------------------
{
  test_vertex_addition() ;
  Vertex *vert = _vertices.data() + _nverts++ ;

  real u = ( _cube[0] ) / ( _cube[0] - _cube[1] ) ;

  vert->x      = (real)_i+u;
  vert->y      = (real) _j ;
  vert->z      = (real) _k ;

  vert->nx = (1-u)*get_x_grad(_i,_j,_k) + u*get_x_grad(_i+1,_j,_k) ;
  vert->ny = (1-u)*get_y_grad(_i,_j,_k) + u*get_y_grad(_i+1,_j,_k) ;
  vert->nz = (1-u)*get_z_grad(_i,_j,_k) + u*get_z_grad(_i+1,_j,_k) ;

  u = (real) sqrt( vert->nx * vert->nx + vert->ny * vert->ny +vert->nz * vert->nz ) ;
  if( u > 0 )
  {
    vert->nx /= u ;
    vert->ny /= u ;
    vert->nz /= u ;
  }


  return _nverts-1 ;
}
//-----------------------------------------------------------------------------

int MarchingCubes::add_y_vertex( )
//-----------------------------------------------------------------------------
{
  test_vertex_addition() ;
  Vertex *vert = _vertices.data() + _nverts++ ;

  real u = ( _cube[0] ) / ( _cube[0] - _cube[3] ) ;

  vert->x      = (real) _i ;
  vert->y      = (real)_j+u;
  vert->z      = (real) _k ;

  vert->nx = (1-u)*get_x_grad(_i,_j,_k) + u*get_x_grad(_i,_j+1,_k) ;
  vert->ny = (1-u)*get_y_grad(_i,_j,_k) + u*get_y_grad(_i,_j+1,_k) ;
  vert->nz = (1-u)*get_z_grad(_i,_j,_k) + u*get_z_grad(_i,_j+1,_k) ;

  u = (real) sqrt( vert->nx * vert->nx + vert->ny * vert->ny +vert->nz * vert->nz ) ;
  if( u > 0 )
  {
    vert->nx /= u ;
    vert->ny /= u ;
    vert->nz /= u ;
  }

  return _nverts-1 ;
}
//-----------------------------------------------------------------------------

int MarchingCubes::add_z_vertex( )
//-----------------------------------------------------------------------------
{
  test_vertex_addition() ;
  Vertex *vert = _vertices.data() + _nverts++ ;

  real u = ( _cube[0] ) / ( _cube[0] - _cube[4] ) ;

  vert->x      = (real) _i ;
  vert->y      = (real) _j ;
  vert->z      = (real)_k+u;

  vert->nx = (1-u)*get_x_grad(_i,_j,_k) + u*get_x_grad(_i,_j,_k+1) ;
  vert->ny = (1-u)*get_y_grad(_i,_j,_k) + u*get_y_grad(_i,_j,_k+1) ;
  vert->nz = (1-u)*get_z_grad(_i,_j,_k) + u*get_z_grad(_i,_j,_k+1) ;

  u = (real) sqrt( vert->nx * vert->nx + vert->ny * vert->ny +vert->nz * vert->nz ) ;
  if( u > 0 )
  {
    vert->nx /= u ;
    vert->ny /= u ;
    vert->nz /= u ;
  }

  return _nverts-1 ;
}


int MarchingCubes::add_c_vertex( )
//-----------------------------------------------------------------------------
{
  test_vertex_addition() ;
  Vertex *vert = _vertices.data() + _nverts++ ;

  real u = 0 ;
  int   vid ;

  vert->x = vert->y = vert->z =  vert->nx = vert->ny = vert->nz = 0 ;

  // Computes the average of the intersection points of the cube
  vid = get_x_vert( _i , _j , _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_y_vert(_i+1, _j , _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_x_vert( _i ,_j+1, _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_y_vert( _i , _j , _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_x_vert( _i , _j ,_k+1) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_y_vert(_i+1, _j ,_k+1) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_x_vert( _i ,_j+1,_k+1) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_y_vert( _i , _j ,_k+1) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_z_vert( _i , _j , _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_z_vert(_i+1, _j , _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_z_vert(_i+1,_j+1, _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }
  vid = get_z_vert( _i ,_j+1, _k ) ;
  if( vid != -1 ) { ++u ; const Vertex &v = _vertices[vid] ; vert->x += v.x ;  vert->y += v.y ;  vert->z += v.z ;  vert->nx += v.nx ; vert->ny += v.ny ; vert->nz += v.nz ; }

  vert->x  /= u ;
  vert->y  /= u ;
  vert->z  /= u ;

  u = (real) sqrt( vert->nx * vert->nx + vert->ny * vert->ny +vert->nz * vert->nz ) ;
  if( u > 0 )
  {
    vert->nx /= u ;
    vert->ny /= u ;
    vert->nz /= u ;
  }

  return _nverts-1 ;
}
//_____________________________________________________________________________
