/*
 * ParX - readcsv.c
 * CSV Input for data nodes
 *
 * Copyright (c) 2009 M.G.Middelhoek <martin@middelhoek.com>
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

#include "datastruct.h"
#include "parx.h"
#include "primtype.h"

inum readcsv(FILE *fp, datatemplate dt) {
    int c;
    char token[1024];
    char *tp;
    int nest, data;
    int cc, ncol, col;
    fnum f;

    datatemplate dti;
    colhead_list head, sweep, hv, he;
    datarow_list dr, dri, dro;
    inum crvid;
    stateflag state;
    inum idx, iv, ie;
    inum ssign0, ssign1, ncurve, ncurves, nflyb;

    head = colheadNIL;
    dr = datarowNIL;
    dti = new_datatemplate(tmstringNIL, head, dr);

    nest = data = ncol = col = cc = 0;
    tp = token;
    tm_lineno = 1;
    crvid = 1;

    while ((c = fgetc(fp)) != EOF) {
        cc++;
        switch (c) {
        case '"':
            nest = !nest;
            continue;
        case ' ':
        case '\t':
        case '\r':
            continue; /* always remove white space */

        case '\n':
            if (cc == 1) { /* empty line separates curves */
                crvid++;
                if (data && dr) {
                    dr->crvid = crvid;
                }
                tm_lineno++;
                cc = 0;
                continue;
            }
            if (nest) { /* eol in string */
                (void)strcpy(tm_errmsg, "open string at eol");
                goto error;
            }
            if (!data) { /* header line defines number of columns */
                ncol = ++col;
            } else if (col != ncol) { /* eol unexpected */
                (void)strcpy(tm_errmsg, "missing data column");
                goto error;
            }
            col = cc = 0;

        case ',':
            if (nest && data) { /* nested number with ',' decimal sep */
                *(tp++) = '.';
                *tp = 0;
                continue;
            }
            if (data == 0) {       /* parsing header */
                if (tp == token) { /* empty header */
                    head = new_colhead(new_tmstring(""), FACT);
                    dti->header = append_colhead_list(dti->header, head);
                } else {          /* parse header */
                    state = FACT; /* determine type */
                    if (strstr(token, ":sw")) {
                        state = SWEEP;
                    }
                    if (strstr(token, ":x")) {
                        if (strstr(token, ":x0")) {
                            state = SWEEP;
                        } else {
                            state = STIM;
                        }
                    }
                    if (strstr(token, ":st"))
                        state = STIM;
                    if (strstr(token, ":y"))
                        state = STIM;
                    if (strstr(token, ":m"))
                        state = MEAS;
                    if (strstr(token, ":c"))
                        state = CALC;
                    if (strstr(token, ":f"))
                        state = FACT;
                    if (strstr(token, ":e"))
                        state = ERR;
                    tp = strchr(token, ':');
                    if (tp) {
                        *tp = '\0';
                    }
                    head = new_colhead(new_tmstring(token), state);
                    dti->header = append_colhead_list(dti->header, head);
                }
            } else {               /* parsing data */
                if (tp == token) { /* column is empty */
                    assert(dr != datarowNIL);
                    dr->row = append_fnum_list(dr->row, (fnum)0.0);
                } else { /* a number */
                    if (sscanf(token, "%le", &f) != 1) {
                        (void)strcpy(tm_errmsg, " illegal number");
                        goto error;
                    }
                    dr->row = append_fnum_list(dr->row, f);
                }
            }
            if (c == '\n') { /* column terminator is eol */
                dr = new_datarow(1, crvid, data + 1, new_fnum_list(),
                                 new_fnum_list());
                dti->data = append_datarow_list(dti->data, dr);
                data++;
                tm_lineno++;
            }
            tp = token;
            col++;
            continue;

        default:
            if (!data && (isalnum(c) || c == '_' ||
                          c == ':')) { /* variable name & type */
                *(tp++) = c;
                *tp = 0;
                continue;
            } else if (data && (isdigit(c) ||
                                strchr("eE+-.", c))) { /* a valid number */
                *(tp++) = c;
                *tp = 0;
                continue;
            } else {
                sprintf(tm_errmsg, "illegal character '%c' in c%d", c, col + 1);
                goto error;
            }
        }
    }

    /* remove any empty rows (last one!) */

    for (dr = dti->data; dr != datarowNIL; dr = dr->next) {
        if (dr->next != datarowNIL && LSTS(dr->next->row) == 0) {
            dri = dr->next;
            dr->next = dr->next->next;
            rfre_datarow(dri);
        }
    }

    /* reorder header, find sweep var */

    sweep = colheadNIL;
    for (head = dti->header, col = 0; head != colheadNIL; head = head->next) {
        if (head->type != ERR) {
            col++;
        }
        if (head->type == SWEEP) {
            if (sweep != colheadNIL) {
                tm_lineno = 1;
                (void)strcpy(tm_errmsg, "multiple sweep variables");
                goto error;
            } else {
                sweep = head;
            }
        }
    }
    if (sweep != colheadNIL) { /* there is a sweep, put it first */
        dt->header = append_colhead_list(dt->header, rdup_colhead(sweep));
    }
    for (head = dti->header; head != colheadNIL; head = head->next) {
        if (head->type == STIM) {
            dt->header = append_colhead_list(dt->header, rdup_colhead(head));
        }
    }
    for (head = dti->header; head != colheadNIL; head = head->next) {
        if (head->type == MEAS || head->type == CALC) {
            dt->header = append_colhead_list(dt->header, rdup_colhead(head));
        }
    }
    for (head = dti->header; head != colheadNIL; head = head->next) {
        if (head->type == FACT) {
            dt->header = append_colhead_list(dt->header, rdup_colhead(head));
        }
    }

    /* create receiving datarows */

    for (dri = dti->data; dri != datarowNIL; dri = dri->next) {
        dro = new_datarow(dri->grpid, dri->crvid, dri->rowid, new_fnum_list(),
                          new_fnum_list());
        dt->data = append_datarow_list(dt->data, dro);
    }

    /* transpose the data */

    for (head = dt->header, idx = 0; head != colheadNIL;
         head = head->next, idx++) {
        for (hv = dti->header, iv = 0; hv != colheadNIL; hv = hv->next, iv++) {
            if (hv->type != ERR && strcmp(head->name, hv->name) == 0) {
                break;
            }
        }
        for (he = dti->header, ie = 0; he != colheadNIL; he = he->next, ie++) {
            if (he->type == ERR && strcmp(head->name, he->name) == 0) {
                break;
            }
        }
        for (dri = dti->data, dro = dt->data; dri != datarowNIL;
             dri = dri->next, dro = dro->next) {
            dro->row = append_fnum_list(
                dro->row, hv == colheadNIL ? (fnum)0.0 : LST(dri->row, iv));
            dro->err = append_fnum_list(
                dro->err, he == colheadNIL ? (fnum)0.0 : LST(dri->row, ie));
        }
    }

    /* determine if data is ordered in curves on sweep variable*/

    ssign1 = 0;
    ncurve = 1;
    ncurves = 1;
    nflyb = 0;

    if (sweep != colheadNIL && dt->data != datarowNIL) {
        for (dr = dt->data; dr->next != datarowNIL; dr = dr->next) {

            if (LST(dr->row, 0) == LST(dr->next->row, 0)) {
                ssign0 = 0;
            } else if (LST(dr->row, 0) < LST(dr->next->row, 0)) {
                ssign0 = 1;
            } else {
                ssign0 = -1;
            }

            if (dr->crvid != dr->next->crvid) { /* curve switch */
                ncurve++;
            }

            if (ssign0 == -ssign1) { /* flyback */
                nflyb++;
                if (dr->crvid != dr->next->crvid) { /* curve switch in sync */
                    ncurves++;
                }
            }
            ssign1 = ssign0;
        }
        nflyb /= 2;

        if (nflyb == (ncurves - 1) && ncurve == ncurves) { /* nothing to do */
        } else if (ncurve !=
                   ncurves) { /* is split in curves, but not on sweep */
        } else {              /* split in curves */
            ssign1 = 0;
            ncurve = 2;
            for (dr = dt->data; dr->next != datarowNIL; dr = dr->next) {
                if (LST(dr->row, 0) == LST(dr->next->row, 0)) {
                    ssign0 = 0;
                } else if (LST(dr->row, 0) < LST(dr->next->row, 0)) {
                    ssign0 = 1;
                } else {
                    ssign0 = -1;
                }
                if (ssign0 == -ssign1) { /* flyback */
                    ncurve++;
                }
                ssign1 = ssign0;
                dr->crvid = ncurve / 2;
            }
            dr->crvid = ncurve / 2;
        }
    }

    rfre_datatemplate(dti);
    return (0);
error:
    rfre_datatemplate(dti);
    return (1);
}
