/* kitty: C++ truth table library
 * Copyright (C) 2017-2020  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file threshold_identification.hpp
  \brief Threshold logic function identification

  \author CS-472 2020 Fall students
*/

#pragma once

#include <vector>
#include <lpsolve/lp_lib.h> /* uncomment this line to include lp_solve */
#include "traits.hpp"

#include <iostream> // need to be deleted
using namespace std;

namespace kitty
{

/*! \brief Threshold logic function identification

  Given a truth table, this function determines whether it is a threshold logic function (TF)
  and finds a linear form if it is. A Boolean function is a TF if it can be expressed as

  f(x_1, ..., x_n) = \sum_{i=1}^n w_i x_i >= T

  where w_i are the weight values and T is the threshold value.
  The linear form of a TF is the vector [w_1, ..., w_n; T].

  \param tt The truth table
  \param plf Pointer to a vector that will hold a linear form of `tt` if it is a TF.
             The linear form has `tt.num_vars()` weight values and the threshold value
             in the end.
  \return `true` if `tt` is a TF; `false` if `tt` is a non-TF.
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
bool is_threshold( const TT& tt, std::vector<int64_t>* plf = nullptr )
{
  std::vector<int64_t> linear_form;

  /* TODO */
  // define some parameters
  auto num_bits = tt.num_bits();
  auto num_vars = tt.num_vars();
  auto tt_fliped = tt;
  int *fliped = new int[num_vars];
  // Step one, use unate check
  for ( auto i = 0u; i < num_vars ; i++ )
  {
    auto fxi     = cofactor1(tt, i);//get cofactor
    auto fxi_not = cofactor0(tt, i);
    //Call function imply to check
	// Check Unate, and flip according to that
	if ( implies( fxi, fxi_not ) )
    {
      tt_fliped = flip( tt_fliped, i );
      fliped[i] = 1;
    }
    else if ( implies( fxi_not, fxi ) )
    {
      fliped[i] = 0;
    }
    else
    {
	  //Else Biunate
	  /* if tt is non-TF: */
      return false;
    }
  }
  // call ILP function, the following code reference to ILP's web
  // Note: the following code is based on web: http://lpsolve.sourceforge.net/5.5/
  // Only a small modification was made, when trying to set up constrain function
	lprec* lp;
	int Ncol, *colno = NULL, ret = 0;
	REAL* row = NULL;

  Ncol = num_vars + 1;
  lp = make_lp( 0, Ncol );

  // Set var names
  if ( ret == 0 )
  {

    // create space large enough for one row 
    colno = (int*)malloc( Ncol * sizeof( *colno ) );
    row = (REAL*)malloc( Ncol * sizeof( *row ) );
    if ( ( colno == NULL ) || ( row == NULL ) )
      ret = 2;
  }
  // The following part is used to set constrains
  for (auto i = 0u; i < num_bits; i++ )
  {
    auto no_bit = i;
	// According to properties of truth_table
    for (auto j = 0u; j < num_vars; j++ )
    {
      colno[j] = j + 1; // Set the name of variables
      row[j] = (no_bit % 2)? 1 : 0; // set the coefficience of variables
	  no_bit = no_bit >> 1;
    }

    colno[num_vars] = num_vars + 1; /* num_vars column */
    row[num_vars] = -1;
    if ( get_bit(tt_fliped,i) )
        add_constraintex( lp, num_vars + 1, row, colno, GE, 0 );
    else
		add_constraintex( lp, num_vars + 1, row, colno, LE, -1 );
  }
  // Set the objective function
  if ( ret == 0 )
  {
    set_add_rowmode( lp, FALSE ); /* rowmode should be turned off again when done building the model */

    for (auto i = 0u; i <= num_vars; i++ )
    {
      colno[i] = i + 1;
      row[i] = 1;
    }
	for (auto i = 1u; i <= num_vars + 1; i++ ) //change them to int
      set_int( lp, i, TRUE );
    /* set the objective in lpsolve */
    if ( !set_obj_fnex( lp, num_vars + 1, row, colno ) )
      ret = 4;
  }

  if ( ret == 0 )
  {
    /* set the object direction to minimize */
    set_minim( lp );

    /* just out of curioucity, now show the model in lp format on screen */
    /* this only works if this is a console application. If not, use write_lp and a filename */
    write_LP( lp, stdout );
    /* write_lp(lp, "model.lp"); */

    /* I only want to see important messages on screen while solving */
    set_verbose( lp, SEVERE );

    /* Now let lpsolve calculate a solution */
    ret = solve( lp );
    if ( ret == OPTIMAL )
      ret = 0;
    else
      ret = 5;
  }

  if ( ret == 0 )
  {
    /* a solution is calculated, now lets get some results */

    /* objective value */
    printf( "Objective value: %f\n", get_objective( lp ) );

    /* variable values */
    get_variables( lp, row );
    int T = row[Ncol - 1];
    for (auto j = 0u; j < num_vars; j++ )
    {
      if ( fliped[j] == 0 )
      {
        linear_form.push_back( row[j] );
      }
      else
      {
        linear_form.push_back( -row[j] );
        T = T - row[j];
      }
    }
    linear_form.push_back( T );
    /* we are done now */
  }

  /* free allocated memory */
  if ( row != NULL )
    free( row );
  if ( colno != NULL )
    free( colno );

  if ( lp != NULL )
  {
    /* clean up such that all used memory by lpsolve is freed */
    delete_lp( lp );
  }
  /* if tt is non-TF: */
  if (ret != 0 )
    return false;

  /* if tt is TF: */
  /* push the weight and threshold values into `linear_form` */
  if ( plf )
  {
    *plf = linear_form;
  }
  return true;
}
}