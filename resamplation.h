/*	$Id$	*/


#ifdef LOOKUP_HACK
#define MAX_DATAVAL 127
#define MIN_DATAVAL -128
#else
#define MAX_DATAVAL 32767
#define MIN_DATAVAL -32768
#endif

#define OVERSHOOT_STEP 50

#ifdef FILTER_INTERPOLATION
#ifndef ENVELOPE_PITCH_MODULATION
#define ENVELOPE_PITCH_MODULATION
#endif
#endif

#if !defined(FILTER_INTERPOLATION) && defined(ENVELOPE_PITCH_MODULATION)
#define CCVARS \
	uint32 cc_count=vp->modulation_counter;
#define MODULATE_NONVIB_PITCH \
		if (!cc_count--) { \
		    cc_count = control_ratio - 1; \
		    if (!update_modulation_signal(v)) \
		        incr = calc_mod_freq(v, incr); \
		}
#define REMEMBER_CC_STATE \
	vp->modulation_counter=cc_count;
#endif

#if defined(CSPLINE_INTERPOLATION)
#ifndef FILTER_INTERPOLATION
# define INTERPVARS      int32   ofsd, v0, v1, v2, v3, temp, overshoot;
# define BUTTERWORTH_COEFFICIENTS \
			overshoot = (int32)src[(int)(se>>FRACTION_BITS)-1] / OVERSHOOT_STEP; \
			if (overshoot < 0) overshoot = -overshoot;
# define RESAMPLATION \
	if (ofs >= se) { \
		int32 delta = (int32)((ofs - se)>>FRACTION_BITS) ; \
        	v1 = (int32)src[(int)(se>>FRACTION_BITS)-1]; \
		v1 -=  (delta+1) * v1 / overshoot; \
        }else  v1 = (int32)src[(int)(ofs>>FRACTION_BITS)]; \
	if (ofs + (1L<<FRACTION_BITS) >= se) { \
		v2 = v1; \
        }else  v2 = (int32)src[(int)(ofs>>FRACTION_BITS)+1]; \
	if(dont_cspline || \
	   ((ofs-(1L<<FRACTION_BITS))<ls)||((ofs+(2L<<FRACTION_BITS))>le)){ \
                *dest++ = (sample_t)(v1 + ((int32)((v2-v1) * (int32)(ofs & FRACTION_MASK)) >> FRACTION_BITS)); \
	}else{ \
		ofsd=ofs; \
                v0 = (int32)src[(int)(ofs>>FRACTION_BITS)-1]; \
                v3 = (int32)src[(int)(ofs>>FRACTION_BITS)+2]; \
                ofs &= FRACTION_MASK; \
                temp=v2; \
		v2 = (6*v2 + \
		      (((( ( (5*v3 - 11*v2 + 7*v1 - v0)* \
		       (int32)ofs) >>FRACTION_BITS)*(int32)ofs)>>(FRACTION_BITS+2))-1))*(int32)ofs; \
                ofs = (1L << FRACTION_BITS) - ofs; \
		v1 = (6*v1 + \
		      ((((((5*v0 - 11*v1 + 7*temp - v3)* \
		       (int32)ofs)>>FRACTION_BITS)*(int32)ofs)>>(FRACTION_BITS+2))-1))*(int32)ofs; \
		v1 = (v1 + v2)/(6L<<FRACTION_BITS); \
		*dest++ = (v1 > MAX_DATAVAL)? MAX_DATAVAL: ((v1 < MIN_DATAVAL)? MIN_DATAVAL: v1); \
		ofs=ofsd; \
	}
#else
# define INTERPVARS      int32   v0, v1, v2, v3, temp, overshoot; \
			float insamp, outsamp, a0, a1, a2, b0, b1, \
			    x0=vp->current_x0, x1=vp->current_x1, y0=vp->current_y0, y1=vp->current_y1; \
			uint32 cc_count=vp->modulation_counter, bw_index=vp->bw_index, ofsdu; \
			sample_t newsample;
# define BUTTERWORTH_COEFFICIENTS \
			a0 = butterworth[bw_index][0]; \
			a1 = butterworth[bw_index][1]; \
			a2 = butterworth[bw_index][2]; \
			b0 = butterworth[bw_index][3]; \
			b1 = butterworth[bw_index][4]; \
			overshoot = (int32)src[(int)(se>>FRACTION_BITS)-1] / OVERSHOOT_STEP; \
			if (overshoot < 0) overshoot = -overshoot;
# define RESAMPLATION \
	if (ofs >= se) { \
		int32 delta = (ofs - se)>>FRACTION_BITS; \
        	v1 = (int32)src[(se>>FRACTION_BITS)-1]; \
		v1 -=  (delta+1) * v1 / overshoot; \
        }else  v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
	if (ofs + (1L<<FRACTION_BITS) >= se) { \
		v2 = v1; \
        }else  v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	if(dont_cspline || \
	   ((ofs-(1L<<FRACTION_BITS))<ls)||((ofs+(2L<<FRACTION_BITS))>le)){ \
		if (!cc_count--) { \
		    cc_count = control_ratio - 1; \
		    if (calc_bw_index(v)) { \
		        bw_index = vp->bw_index; \
			a0 = butterworth[bw_index][0]; \
			a1 = butterworth[bw_index][1]; \
			a2 = butterworth[bw_index][2]; \
			b0 = butterworth[bw_index][3]; \
			b1 = butterworth[bw_index][4]; \
		    } \
		    incr = calc_mod_freq(v, incr); \
		} \
		if (dont_filter_melodic) bw_index = 0; \
                newsample = (sample_t)(v1 + ((int32)((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS)); \
	        if (bw_index) { \
                    insamp = (float)newsample; \
		    outsamp = a0 * insamp + a1 * x0 + a2 * x1 - b0 * y0 - b1 * y1; \
		    x1 = x0; \
		    x0 = insamp; \
		    y1 = y0; \
		    y0 = outsamp; \
		    newsample = (outsamp > MAX_DATAVAL)? MAX_DATAVAL: \
			((outsamp < MIN_DATAVAL)? MIN_DATAVAL: (sample_t)outsamp); \
	        } \
	}else{ \
		ofsdu=ofs; \
                v0 = (int32)src[(int)(ofs>>FRACTION_BITS)-1]; \
                v3 = (int32)src[(int)(ofs>>FRACTION_BITS)+2]; \
                ofs &= FRACTION_MASK; \
                temp=v2; \
		v2 = (6*v2 + \
		      ((((((5*v3 - 11*v2 + 7*v1 - v0)* \
		       ofs)>>FRACTION_BITS)*ofs)>>(FRACTION_BITS+2))-1))*ofs; \
                ofs = (1L << FRACTION_BITS) - ofs; \
		v1 = (6*v1 + \
		      ((((((5*v0 - 11*v1 + 7*temp - v3)* \
		       ofs)>>FRACTION_BITS)*ofs)>>(FRACTION_BITS+2))-1))*ofs; \
		v1 = (v1 + v2)/(6L<<FRACTION_BITS); \
		if (!cc_count--) { \
		    cc_count = control_ratio - 1; \
		    if (calc_bw_index(v)) { \
			bw_index = vp->bw_index; \
			a0 = butterworth[bw_index][0]; \
			a1 = butterworth[bw_index][1]; \
			a2 = butterworth[bw_index][2]; \
			b0 = butterworth[bw_index][3]; \
			b1 = butterworth[bw_index][4]; \
		    } \
		    incr = calc_mod_freq(v, incr); \
		} \
		if (dont_filter_melodic) bw_index = 0; \
		newsample = (v1 > MAX_DATAVAL)? MAX_DATAVAL: ((v1 < MIN_DATAVAL)? MIN_DATAVAL: (sample_t)v1); \
		if (bw_index) { \
                    insamp = (float)newsample; \
		    outsamp = a0 * insamp + a1 * x0 + a2 * x1 - b0 * y0 - b1 * y1; \
		    x1 = x0; \
		    x0 = insamp; \
		    y1 = y0; \
		    y0 = outsamp; \
		    newsample = (outsamp > MAX_DATAVAL)? MAX_DATAVAL: \
			((outsamp < MIN_DATAVAL)? MIN_DATAVAL: (sample_t)outsamp); \
		} \
		ofs=ofsdu; \
	} \
	*dest++ = newsample;
#define REMEMBER_FILTER_STATE \
	vp->current_x0=x0; \
	vp->current_x1=x1; \
	vp->current_y0=y0; \
	vp->current_y1=y1; \
	vp->bw_index=bw_index; \
	vp->modulation_counter=cc_count;
#endif
#elif defined(LAGRANGE_INTERPOLATION)
# define INTERPVARS      int32   ofsd, v0, v1, v2, v3, overshoot;
# define BUTTERWORTH_COEFFICIENTS \
			overshoot = src[(se>>FRACTION_BITS)-1] / OVERSHOOT_STEP; \
			if (overshoot < 0) overshoot = -overshoot;
# define RESAMPLATION \
	if (ofs >= se) { \
		int32 delta = (ofs - se)>>FRACTION_BITS ; \
        	v1 = (int32)src[(se>>FRACTION_BITS)-1]; \
		v1 -=  (delta+1) * v1 / overshoot; \
        }else  v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
	if (ofs + (1L<<FRACTION_BITS) >= se) { \
		v2 = v1; \
        }else  v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	if(dont_cspline || \
	   ((ofs-(1L<<FRACTION_BITS))<ls)||((ofs+(2L<<FRACTION_BITS))>le)){ \
                *dest++ = (sample_t)(v1 + ((int32)((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS)); \
	}else{ \
                v0 = (int32)src[(ofs>>FRACTION_BITS)-1]; \
                v3 = (int32)src[(ofs>>FRACTION_BITS)+2]; \
                ofsd = (int32)(ofs & FRACTION_MASK) + (1L << FRACTION_BITS); \
                v1 = v1*ofsd>>FRACTION_BITS; \
                v2 = v2*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd>>FRACTION_BITS; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = v0*ofsd>>FRACTION_BITS; \
                v2 = v2*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd>>FRACTION_BITS; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = v0*ofsd>>FRACTION_BITS; \
                v1 = v1*ofsd>>FRACTION_BITS; \
                v3 = v3*ofsd; \
                ofsd -= (1L << FRACTION_BITS); \
                v0 = (v3 - v0*ofsd)/(6L << FRACTION_BITS); \
                v1 = (v1 - v2)*ofsd>>(FRACTION_BITS+1); \
		v1 += v0; \
		*dest++ = (v1 > MAX_DATAVAL)? MAX_DATAVAL: ((v1 < MIN_DATAVAL)? MIN_DATAVAL: v1); \
	}
#elif defined(LINEAR_INTERPOLATION)
# if defined(LOOKUP_HACK) && defined(LOOKUP_INTERPOLATION)
# define BUTTERWORTH_COEFFICIENTS \
			overshoot = src[(se>>FRACTION_BITS)-1] / OVERSHOOT_STEP; \
			if (overshoot < 0) overshoot = -overshoot;
# define INTERPVARS     int32 overshoot;
#   define RESAMPLATION \
	if (ofs >= se) { \
		int32 delta = (ofs - se)>>FRACTION_BITS ; \
        	v1 = (int32)src[(se>>FRACTION_BITS)-1]; \
		v1 -=  (delta+1) * v1 / overshoot; \
        }else  v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
	if (ofs + (1L<<FRACTION_BITS) >= se) { \
		v2 = v1; \
        }else  v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	*dest++ = (sample_t)(v1 + (iplookup[(((v2-v1)<<5) & 0x03FE0) | \
           ((int32)(ofs & FRACTION_MASK) >> (FRACTION_BITS-5))]));
# else
# define BUTTERWORTH_COEFFICIENTS \
			overshoot = src[(se>>FRACTION_BITS)-1] / OVERSHOOT_STEP; \
			if (overshoot < 0) overshoot = -overshoot;
#  define INTERPVARS int32 v1, v2, overshoot=0;
#   define RESAMPLATION \
	if (ofs >= se) { \
		int32 delta = (ofs - se)>>FRACTION_BITS ; \
        	v1 = (int32)src[(se>>FRACTION_BITS)-1]; \
		v1 -=  (delta+1) * v1 / overshoot; \
        }else  v1 = (int32)src[(ofs>>FRACTION_BITS)]; \
	if (ofs + (1L<<FRACTION_BITS) >= se) { \
		v2 = v1; \
        }else  v2 = (int32)src[(ofs>>FRACTION_BITS)+1]; \
	*dest++ = (sample_t)(v1 + ((int32)((v2-v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
# endif
#else
/* Earplugs recommended for maximum listening enjoyment */
#  define RESAMPLATION \
	if (ofs >= se) *dest++ = 0; \
	else *dest++=src[ofs>>FRACTION_BITS];
#  define INTERPVARS int32 overshoot=0;
#endif

