/*
 * ParX - stim2dat.c
 * Simulation Setup
 *
 * Copyright (c) 1990 M.G.Middelhoek <martin@middelhoek.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "parx.h"
#include "error.h"
#include "stim2dat.h"

static colhead_list makeheader(stimtemplate_list st, modeltemplate mt);
static datarow_list makedata(colhead_list header, stimtemplate_list st);
static boolean teststim(stimtemplate_list st);
static boolean test_setup(stimtemplate st);

/************************************************************************/

datatemplate stim2dat(stimtemplate_list st, modeltemplate mt) {
    datatemplate dt;
    colhead_list header;
    datarow_list dat;
    
    dt = datatemplateNIL;
    
    /* test stimulus units for legality */
    
    if (teststim(st) == FALSE)
        return (dt);
    
    /* setup the data captions */
    
    if ((header = makeheader(st, mt)) == colheadNIL)
        return (dt);
    
    /* fill the data table */
    
    dat = makedata(header, st);
    
    dt = new_datatemplate(new_tmstring("stimulus derived data"), header, dat);
    
    return (dt);
}

/* test the stimulus node */

boolean teststim(stimtemplate_list st) {
    for (; st != stimtemplateNIL; st = st->next) {
        if (test_setup(st) == FALSE) {
            errcode = ILL_SETUP_SERR;
            error(st->name);
            return (FALSE);
        }
    }
    return (TRUE);
}

/* construct the header captions list */

colhead_list makeheader(stimtemplate_list st, modeltemplate mt) {
    xspec_list xext;
    colhead_list header;
    stimtemplate_list s, s_max;
    inum nint_max;
    
    
    /* find the sweep variable, largest value of intervals */
    
    for (s = st, s_max = stimtemplateNIL, nint_max = 0; s != stimtemplateNIL; s = s->next) {
        for (xext = mt->xext; xext != xspecNIL; xext = xext->next) {
            if (strcmp(s->name, xext->name) == 0)
                break;
        }
        
        if (xext != xspecNIL && (s->nint > nint_max)) { /* is it connected? */
            nint_max = s->nint;
            s_max = s;
        }
    }
    
    header = new_colhead_list();
    
    if (s_max != stimtemplateNIL) /* sweep in first column */
        header = append_colhead_list(header, new_colhead(new_tmstring(s_max->name), SWEEP));
    else return (header);
    
    /* find the model external for all the stimulus units */
    
    for (s = st; s != stimtemplateNIL; s = s->next) {
        
        if (s == s_max) continue;
        
        for (xext = mt->xext; xext != xspecNIL; xext = xext->next) {
            if (strcmp(s->name, xext->name) == 0)
                break;
        }
        
        if (xext != xspecNIL) { /* stimulus found */
            header = append_colhead_list(header,
                                         new_colhead(new_tmstring(xext->name), STIM));
        } /* ignore unconnected stimuli */
    }
    
    /* find the specified stimulus unit for all externals of the model */
    
    for (xext = mt->xext; xext != xspecNIL; xext = xext->next) {
        
        /* find model external in stimulus list */
        for (s = st; s != stimtemplateNIL; s = s->next)
            if (strcmp(s->name, xext->name) == 0)
                break;
        
        if (s == stimtemplateNIL) { /* stimulus not found */
            header = append_colhead_list(header,
                                         new_colhead(new_tmstring(xext->name), UNKN));
        } /* unconnected means unknown */
    }
    
    /* do not check for unused stimulus units */
    /* this is a feature not a bug */
    
    return (header);
}

/* check the setup of the stimulus units for consistency */

boolean test_setup(stimtemplate st) {
    if (st->nint < 0) return (FALSE);
    /*
     if (st->lval > st->uval) return(FALSE);
     */
    switch (st->scale) {
        case SLOG:
        case SLN:
            if (st->lval <= 0.0 || st->uval <= 0.0) return (FALSE);
            break;
        case ALOG:
        case ALN:
            if ((SIGN(st->lval) != SIGN(st->uval)) ||
                st->lval == 0.0 || st->uval == 0.0)
                return (FALSE);
            break;
        default: break;
    }
    return (TRUE);
}


/* use stimulus info to fill data table with initial values */
/* generate all permutations of stimulus values             */

/* first column changes fastest	                            */

datarow_list makedata(colhead_list header, stimtemplate_list st) {
    stimtemplate_list s;
    colhead_list h;
    datarow_list data, dr;
    
    inum np;
    fnum lowerb, upperb, step, val;
    inum i, n, rep, same, sig, crvid;
    
    /* calculate the number of data points */
    
    np = 0; /* number of points */
    
    for (h = header; h != colheadNIL; h = h->next) {
        
        if ((h->type == STIM) || (h->type == SWEEP)) {
            s = find_stim(st, h->name);
            assert(s != stimtemplateNIL);
            if (np == 0) {
                np = s->nint + 1;
            } else {
                np *= s->nint + 1;
            }
        }
    }
    
    /* create data table */
    
    data = new_datarow_list();
    
    for (i = 1; i <= np; i++) {
        data = append_datarow_list(data, new_datarow(0, 1, i,
                                                     new_fnum_list(), new_fnum_list()));
    }
    
    rep = 0; /* number of times one value is repeated in a column */
    
    /* fill all columns, from left to right */
    
    for (h = header; h != colheadNIL; h = h->next) {
        
        if (h->type == UNKN) { /* skip unknown externals */
            for (dr = data; dr != datarowNIL; dr = dr->next) {
                dr->row = append_fnum_list(dr->row, 0.0);
                dr->err = append_fnum_list(dr->err, 0.0);
            }
            continue;
        }
        
        s = find_stim(st, h->name);
        assert(s != stimtemplateNIL);
        
        /* calculate bounds, step size and repeat count */
        
        sig = SIGN(s->lval); /* sign bit */
        
        /* scale bounds and step size */
        
        switch (s->scale) {
            case SLIN:
            case ALIN:
                lowerb = s->lval;
                upperb = s->uval;
                break;
            case SLOG:
            case ALOG:
                lowerb = log10(fabs(s->lval));
                upperb = log10(fabs(s->uval));
                break;
            case SLN:
            case ALN:
                lowerb = log(fabs(s->lval));
                upperb = log(fabs(s->uval));
                break;
        }
        
        if (s->nint > 0) {
            step = (upperb - lowerb) / s->nint;
        } else {
            step = 0.0;
        }
        
        /* fill the column assigned to the unit */
        
        same = 0;
        n = 0;
        crvid = 1;
        
        for (dr = data; dr != datarowNIL; dr = dr->next) {
            
            switch (s->scale) { /* un-scale */
                case SLIN:
                case ALIN:
                    val = lowerb + n * step;
                    break;
                case SLOG:
                case ALOG:
                    val = pow(10.0, lowerb + n * step);
                    val = sig > 0 ? val : -val;
                    break;
                case SLN:
                case ALN:
                    val = exp(lowerb + n * step);
                    val = sig > 0 ? val : -val;
                    break;
            }
            dr->row = append_fnum_list(dr->row, val);
            dr->err = append_fnum_list(dr->err, 0.0);
            if (rep == 0) {
                dr->crvid = crvid;
            }
            
            if (++same >= rep) {
                same = 0;
                n++;
                if (n > s->nint) {
                    n = 0;
                    crvid++;
                }
            }
        }
        
        if (rep == 0) {
            rep = s->nint + 1;
        } else {
            rep = rep * (s->nint + 1); /* repeat for next column */
        }
    }
    return (data);
}
