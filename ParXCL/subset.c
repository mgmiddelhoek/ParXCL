/*
 * ParX - subset.c
 * Select a Subset of the data
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
#include "subset.h"

typedef struct _indextab {/* index table to data rows */
    datarow dr;
    boolean del; /* deleted ? */
} indextab;

datatemplate subset_data(meastemplate_list meast, datatemplate datat) {
    meastemplate_list ml;
    inum mtype;
    tmstring mname;
    fnum mlval, muval;
    inum mlvali, muvali;
    inum msubs;
    colhead_list h;
    inum ix;
    datarow dr;
    inum nr, delnr, i;
    indextab *idx;
    tmstring si, s_info;
    colhead_list s_head;
    datarow_list s_data;
    datarow n_data, l_data;
    
    /* get the index of all externals */
    
    for (ml = meast; ml != meastemplateNIL; ml = ml->next) {
        
        mtype = ml->mtype;
        
        if (mtype < 0) { /* external reference */
            
            mname = ml->name;
            
            for (h = datat->header, ix = 0; h != colheadNIL; h = h->next, ix++) {
                if (strcmp(mname, h->name) == 0)
                    break;
            }
            
            if (h == colheadNIL) {
                errcode = NO_KEY_SERR;
                error(mname);
                return (datatemplateNIL);
            }
            
            if (h->type == UNKN) {
                errcode = UNKN_VAR_SERR;
                error(mname);
                return (datatemplateNIL);
            }
            
            ml->mtype = -(ix + 1); /* externals are coded negative */
        }
    }
    
    dr = datat->data;
    
    for (nr = 0; dr != datarowNIL; nr++, dr = dr->next); /* count the points */
    
    idx = TM_MALLOC(indextab *, nr * sizeof (indextab)); /* allocate index table */
    
    dr = datat->data;
    
    for (i = 0; dr != datarowNIL; i++, dr = dr->next) { /* initialize index */
        idx[i].del = FALSE;
        idx[i].dr = dr;
    }
    
    for (ml = meast; ml != meastemplateNIL; ml = ml->next) { /* select points */
        
        mtype = ml->mtype;
        mlval = ml->lval;
        muval = ml->uval;
        msubs = ml->subs;
        msubs = (msubs == 0) ? 1 : msubs;
        if (mtype >= 0) {
            mlvali = roundi(mlval);
            muvali = roundi(muval);
        } else {
            /* slightly expand the selected region, bounds may be truncated */
            mlval -= 1e-6 * fabs(mlval);
            muval += 1e-6 * fabs(muval);
            mlvali = roundi(mlval);
            muvali = roundi(muval);
            
        }
        
        for (i = 0; i < nr; i++) { /* for all data rows */
            
            if (idx[i].del == TRUE) continue; /* already deleted */
            
            if (mtype == 0) { /* group id */
                idx[i].del = ((idx[i].dr)->grpid < mlvali) ? TRUE : idx[i].del;
                idx[i].del = ((idx[i].dr)->grpid > muvali) ? TRUE : idx[i].del;
            }
            
            if (mtype > 0) { /* curve id */
                idx[i].del = (((idx[i].dr)->crvid) < mlvali) ? TRUE : idx[i].del;
                idx[i].del = (((idx[i].dr)->crvid) > muvali) ? TRUE : idx[i].del;
                idx[i].del = (((((idx[i].dr)->crvid)) % msubs) != 0) ? TRUE : idx[i].del;
            }
            
            if (mtype < 0) { /* external */
                idx[i].del = ((LST((idx[i].dr)->row, -mtype - 1)) < mlval) ? TRUE : idx[i].del;
                idx[i].del = ((LST((idx[i].dr)->row, -mtype - 1)) > muval) ? TRUE : idx[i].del;
            }
        }
    }
    
    s_data = new_datarow_list();
    l_data = s_data;
    
    for (i = 0, delnr = 0; i < nr; i++) { /* for all data rows */
        
        if (idx[i].del == TRUE) {
            delnr++;
            continue;
        }
        
        /* add to new data list */
        
        n_data = rdup_datarow(idx[i].dr);
        n_data->next = datarow_listNIL;
        
        if (s_data == datarow_listNIL)
            s_data = n_data;
        else
            l_data->next = n_data;
        
        l_data = n_data;
    }
    
    if ((nr - delnr) != nr)
        fprintf(error_stream, "\ndata points selected: %ld out of %ld\n",
                (long) (nr - delnr), (long) nr);
    
    TM_FREE(idx); /* free the index table */
    
    si = TM_MALLOC(char *, (strlen(datat->info) + 13) * sizeof (char));
    si = strcpy(si, ((nr - delnr) != nr) ? "Subset of : " : "");
    si = strcat(si, datat->info);
    s_info = new_tmstring(si);
    
    s_head = rdup_colhead_list(datat->header);
    
    return new_datatemplate(s_info, s_head, s_data);
}
