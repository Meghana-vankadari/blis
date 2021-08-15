/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin
   Copyright (C) 2018 - 2020, Advanced Micro Devices, Inc.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"

static void_fp bli_l3_ind_oper_fp[BLIS_NUM_IND_METHODS][BLIS_NUM_LEVEL3_OPS] =
{
        /*   gemm          gemmt         hemm          herk          her2k         symm
             syrk          syr2k         trmm3         trmm          trsm  */
/* 3mh  */ { bli_gemm3mh,  NULL,         bli_hemm3mh,  NULL,         NULL,         bli_symm3mh,
             NULL,         NULL,         bli_trmm33mh, NULL,         NULL         },
/* 3m1  */ { bli_gemm3m1,  NULL,         bli_hemm3m1,  NULL,         NULL,         bli_symm3m1,
             NULL,         NULL,         bli_trmm33m1, bli_trmm3m1,  bli_trsm3m1  },
/* 4mh  */ { bli_gemm4mh,  NULL,         bli_hemm4mh,  NULL,         NULL,         bli_symm4mh,
             NULL,         NULL,         bli_trmm34mh, NULL,         NULL         },
/* 4mb  */ { bli_gemm4mb,  NULL,         NULL,         NULL,         NULL,         NULL,
             NULL,         NULL,         NULL,         NULL,         NULL         },
/* 4m1  */ { bli_gemm4m1,  NULL,         bli_hemm4m1,  NULL,         NULL,         bli_symm4m1,
             NULL,         NULL,         bli_trmm34m1, bli_trmm4m1,  bli_trsm4m1  },
/* 1m   */ { bli_gemm1m,   NULL,         bli_hemm1m,   NULL,         NULL,         bli_symm1m,
             NULL,         NULL,         bli_trmm31m,  bli_trmm1m,   bli_trsm1m   },
/* nat  */ { bli_gemmnat,  bli_gemmtnat, bli_hemmnat,  NULL,         NULL,         bli_symmnat,
             NULL,         NULL,         bli_trmm3nat, bli_trmmnat,  bli_trsmnat  },
};

//
// NOTE: "2" is used instead of BLIS_NUM_FP_TYPES/2.
//
// BLIS provides APIs to modify this state during runtime. So, it's possible for one
// application thread to modify the state before another starts the corresponding
// BLIS operation. This is solved by making the induced method status array local to
// threads.

static BLIS_THREAD_LOCAL
bool bli_l3_ind_oper_st[BLIS_NUM_IND_METHODS][BLIS_NUM_LEVEL3_OPS][2] =
{
        /*   gemm           gemmt          hemm           herk           her2k          symm
             syrk           syr2k          trmm3          trmm           trsm  */
        /*    c     z    */
/* 3mh  */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* 3m1  */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* 4mh  */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* 4mb  */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* 4m1  */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* 1m   */ { {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE},
             {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}, {FALSE,FALSE}  },
/* nat  */ { {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},
             {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE},   {TRUE,TRUE}    },
};

// -----------------------------------------------------------------------------

#undef  GENFUNC
#define GENFUNC( opname, optype ) \
\
void_fp PASTEMAC(opname,ind_get_avail)( num_t dt ) \
{ \
	return bli_ind_oper_get_avail( optype, dt ); \
}
/*
bool PASTEMAC(opname,ind_has_avail)( num_t dt )
{
	return bli_ind_oper_has_avail( optype, dt );
}
*/

GENFUNC( gemm, BLIS_GEMM )
GENFUNC( gemmt, BLIS_GEMMT )
GENFUNC( hemm, BLIS_HEMM )
GENFUNC( symm, BLIS_SYMM )
GENFUNC( trmm3, BLIS_TRMM3 )
GENFUNC( trmm, BLIS_TRMM )
GENFUNC( trsm, BLIS_TRSM )

// -----------------------------------------------------------------------------

#if 0
bool bli_l3_ind_oper_is_avail( opid_t oper, ind_t method, num_t dt )
{
	void_fp func;
	bool    stat;

	// If the datatype is real, it is never available.
	if ( !bli_is_complex( dt ) ) return FALSE;

	func = bli_l3_ind_oper_get_func( oper, method );
	stat = bli_l3_ind_oper_get_enable( oper, method, dt );

	return ( func != NULL && stat == TRUE );
}
#endif

// -----------------------------------------------------------------------------

ind_t bli_l3_ind_oper_find_avail( opid_t oper, num_t dt )
{
	bli_init_once();

	ind_t im;

	// If the datatype is real, return native execution.
	if ( !bli_is_complex( dt ) ) return BLIS_NAT;

	// If the operation is not level-3, return native execution.
	if ( !bli_opid_is_level3( oper ) ) return BLIS_NAT;

	// Iterate over all induced methods and search for the first one
	// that is available (ie: both implemented and enabled) for the
	// current operation and datatype.
	for ( im = 0; im < BLIS_NUM_IND_METHODS; ++im )
	{
		void_fp func = bli_l3_ind_oper_get_func( oper, im );
		bool    stat = bli_l3_ind_oper_get_enable( oper, im, dt );

		if ( func != NULL &&
		     stat == TRUE ) return im;
	}

	// This return statement should never execute since the native index
	// should be found even if all induced methods are unavailable. We
	// include it simply to avoid a compiler warning.
	return BLIS_NAT;
}

// -----------------------------------------------------------------------------

void bli_l3_ind_set_enable_dt( ind_t method, num_t dt, bool status )
{
	opid_t iop;

	if ( !bli_is_complex( dt ) ) return;

	// Iterate over all level-3 operation ids.
	for ( iop = 0; iop < BLIS_NUM_LEVEL3_OPS; ++iop )
	{
		bli_l3_ind_oper_set_enable( iop, method, dt, status );
	}
}

// -----------------------------------------------------------------------------

void bli_l3_ind_oper_enable_only( opid_t oper, ind_t method, num_t dt )
{
	ind_t im;

	if ( !bli_is_complex( dt ) ) return;
	if ( !bli_opid_is_level3( oper ) ) return;

	for ( im = 0; im < BLIS_NUM_IND_METHODS; ++im )
	{
		// Native execution should always stay enabled.
		if ( im == BLIS_NAT ) continue;

		// When we come upon the requested method, enable it for the given
		// operation and datatype. Otherwise, disable it.
		if ( im == method )
			bli_l3_ind_oper_set_enable( oper, im, dt, TRUE );
		else
			bli_l3_ind_oper_set_enable( oper, im, dt, FALSE );
	}
}

void bli_l3_ind_oper_set_enable_all( opid_t oper, num_t dt, bool status )
{
	ind_t im;

	if ( !bli_is_complex( dt ) ) return;
	if ( !bli_opid_is_level3( oper ) ) return;

	for ( im = 0; im < BLIS_NUM_IND_METHODS; ++im )
	{
		// Native execution should always stay enabled.
		if ( im != BLIS_NAT )
			bli_l3_ind_oper_set_enable( oper, im, dt, status );
	}
}

// -----------------------------------------------------------------------------

// A mutex to allow synchronous access to the bli_l3_ind_oper_st array.
static bli_pthread_mutex_t oper_st_mutex = BLIS_PTHREAD_MUTEX_INITIALIZER;

void bli_l3_ind_oper_set_enable( opid_t oper, ind_t method, num_t dt, bool status )
{
	num_t idt;

	if ( !bli_is_complex( dt ) ) return;
	if ( !bli_opid_is_level3( oper ) ) return;

	// Disallow changing status of native execution.
	if ( method == BLIS_NAT ) return;

	idt = bli_ind_map_cdt_to_index( dt );

	// Acquire the mutex protecting bli_l3_ind_oper_st.
	bli_pthread_mutex_lock( &oper_st_mutex );

	// BEGIN CRITICAL SECTION
	{
		bli_l3_ind_oper_st[ method ][ oper ][ idt ] = status;
	}
	// END CRITICAL SECTION

	// Release the mutex protecting bli_l3_ind_oper_st.
	bli_pthread_mutex_unlock( &oper_st_mutex );
}

bool bli_l3_ind_oper_get_enable( opid_t oper, ind_t method, num_t dt )
{
	num_t idt = bli_ind_map_cdt_to_index( dt );
	bool  r_val;

	{
		r_val = bli_l3_ind_oper_st[ method ][ oper ][ idt ];
	}

	return r_val;
}

// -----------------------------------------------------------------------------

void_fp bli_l3_ind_oper_get_func( opid_t oper, ind_t method )
{
	return bli_l3_ind_oper_fp[ method ][ oper ];
}

