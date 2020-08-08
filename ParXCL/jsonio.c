/*
 * ParX - jsonio.c
 * JSON I/O for database nodes
 *
 * Copyright (c) 2012 M.G.Middelhoek <martin@middelhoek.com>
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
#include "datatpl.h"
#include "error.h"
#include "jsonio.h"
#include "parx.h"
#include "primtype.h"

/**
 * Convert a stateflag to a string representation
 *
 * @param s stateflag
 * @return string
 */
char *stateflag2string(stateflag s) {
    switch (s) {
    case UNKN:
        return ("unkn");
    case MEAS:
        return ("meas");
    case CALC:
        return ("calc");
    case FACT:
        return ("fact");
    case STIM:
        return ("stim");
    case SWEEP:
        return ("sweep");
    case ERR:
        return ("err");
    default:
        return ("null");
    }
}

/**
 * Covert a string back to a stateflag
 *
 * @param s string
 * @return stateflag
 */
stateflag string2stateflag(tmstring s) {

    if (strcmp(s, "unkn") == 0) {
        return (UNKN);
    }
    if (strcmp(s, "meas") == 0) {
        return (MEAS);
    }
    if (strcmp(s, "calc") == 0) {
        return (CALC);
    }
    if (strcmp(s, "fact") == 0) {
        return (FACT);
    }
    if (strcmp(s, "stim") == 0) {
        return (STIM);
    }
    if (strcmp(s, "sweep") == 0) {
        return (SWEEP);
    }
    if (strcmp(s, "err") == 0) {
        return (ERR);
    }
    return (UNKN);
}

/**
 * Write a datatemplate to a file in Json-format
 *
 * @param fp file
 * @param dt datatemplate
 */
void write_data_json(FILE *fp, datatemplate dt) {
    cJSON *json_root, *header, *data, *key, *row, *val, *err;
    colhead_list hp;
    datarow_list dr;
    fnum_list flist;
    unsigned int i;
    char *json_out;

    if (dt == datatemplateNIL || dt->info == tmstringNIL) {
        return;
    }

    json_root = cJSON_CreateObject();

    if (dt->info != tmstringNIL) {
        cJSON_AddItemToObject(json_root, "info", cJSON_CreateString(dt->info));
    } else {
        cJSON_AddItemToObject(json_root, "info",
                              cJSON_CreateString("no information available"));
    }

    cJSON_AddItemToObject(json_root, "header", header = cJSON_CreateArray());
    hp = dt->header;
    while (hp != colheadNIL) {
        cJSON_AddItemToArray(header, key = cJSON_CreateObject());
        cJSON_AddItemToObject(key, "name", cJSON_CreateString(hp->name));
        cJSON_AddItemToObject(key, "type",
                              cJSON_CreateString(stateflag2string(hp->type)));
        hp = hp->next;
    }

    cJSON_AddItemToObject(json_root, "data", data = cJSON_CreateArray());
    dr = dt->data;
    while (dr != datarowNIL) {
        cJSON_AddItemToArray(data, row = cJSON_CreateObject());
        cJSON_AddItemToObject(row, "grpid", cJSON_CreateNumber(dr->grpid));
        cJSON_AddItemToObject(row, "crvid", cJSON_CreateNumber(dr->crvid));
        cJSON_AddItemToObject(row, "rowid", cJSON_CreateNumber(dr->rowid));
        cJSON_AddItemToObject(row, "val", val = cJSON_CreateArray());
        cJSON_AddItemToObject(row, "err", err = cJSON_CreateArray());

        flist = dr->row;
        for (i = 0; i < flist->sz; i++) {
            cJSON_AddItemToArray(val, cJSON_CreateNumber(flist->arr[i]));
        }

        flist = dr->err;
        for (i = 0; i < flist->sz; i++) {
            cJSON_AddItemToArray(err, cJSON_CreateNumber(flist->arr[i]));
        }

        dr = dr->next;
    }

    json_out = cJSON_Print(json_root);
    cJSON_Delete(json_root);
    fprintf(fp, "%s\n", json_out);
    free(json_out);

    return;
}

/**
 * Read a datatemplate from a file in Json-format
 *
 * @param fp file
 * @param dt datatemplate
 * @return success
 */
inum read_data_json(FILE *fp, datatemplate *dt) {
    char *json;
    cJSON *root, *header, *data, *key, *row;
    cJSON *info, *name, *type, *grpid, *crvid, *rowid;
    cJSON *val, *err, *v, *e;

    long len;
    inum i, j, hnum, dnum, vnum;
    colhead nw;
    tmstring l_info;
    colhead_list l_header;
    datarow_list l_data;
    fnum_list vl, el;
    datarow nwrow;
    inum headers, values;
    boolean err_headers = FALSE;
    boolean err_values = FALSE;
    boolean err_rows = FALSE;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    json = malloc(len + 1);
    fread(json, 1, len, fp);

    root = cJSON_Parse(json);
    if (!root) {
        sprintf(tm_errmsg, "JSON syntax error before: [%s]",
                cJSON_GetErrorPtr());
        return (1);
    }

    info = cJSON_GetObjectItem(root, "info");
    if (info) {
        l_info = new_tmstring(info->valuestring);
    } else {
        l_info = new_tmstring("no information available");
    }

    header = cJSON_GetObjectItem(root, "header");
    l_header = new_colhead_list();
    hnum = cJSON_GetArraySize(header);
    headers = 0;
    for (i = 0; i < hnum; i++) {
        key = cJSON_GetArrayItem(header, i);
        name = cJSON_GetObjectItem(key, "name");
        type = cJSON_GetObjectItem(key, "type");
        if (name && type) {
            nw = new_colhead(new_tmstring(name->valuestring),
                             string2stateflag(type->valuestring));
            l_header = append_colhead_list(l_header, nw);
            headers++;
        } else {
            err_headers = TRUE;
        }
    }

    data = cJSON_GetObjectItem(root, "data");
    dnum = cJSON_GetArraySize(data);
    l_data = new_datarow_list();
    for (i = 0; i < dnum; i++) {

        row = cJSON_GetArrayItem(data, i);
        if (row) {
            grpid = cJSON_GetObjectItem(row, "grpid");
            crvid = cJSON_GetObjectItem(row, "crvid");
            rowid = cJSON_GetObjectItem(row, "rowid");
            val = cJSON_GetObjectItem(row, "val");
            err = cJSON_GetObjectItem(row, "err");

            if (grpid && crvid && rowid && val && err) {

                vl = new_fnum_list();
                el = new_fnum_list();

                vnum = cJSON_GetArraySize(val);
                values = 0;
                for (j = 0; j < vnum; j++) {
                    v = cJSON_GetArrayItem(val, j);
                    e = cJSON_GetArrayItem(err, j);
                    if (v && e) {
                        vl = append_fnum_list(vl, v->valuedouble);
                        el = append_fnum_list(el, e->valuedouble);
                        values++;
                    } else {
                        err_values = TRUE;
                    }
                }
                if (values != headers) {
                    err_values = TRUE;
                }
                nwrow = new_datarow(grpid->valueint, crvid->valueint,
                                    rowid->valueint, vl, el);
                l_data = append_datarow_list(l_data, nwrow);
            } else {
                err_rows = TRUE;
            }
        } else {
            err_rows = TRUE;
        }
    }

    *dt = new_datatemplate(l_info, l_header, l_data);

    cJSON_Delete(root);
    free(json);

    if (err_headers == TRUE || err_rows == TRUE || err_values == TRUE) {
        sprintf(tm_errmsg, "JSON structure error");
        return (1);
    }
    return (0);
}

/**
 * Convert a parmval-tag to a string
 *
 * @param tag parameter tag
 * @return string
 */
char *parmtag2string(tags_parmval tag) {
    switch (tag) {
    case TAGPunkn:
        return ("unkn");
    case TAGPmeas:
        return ("meas");
    case TAGPcalc:
        return ("calc");
    case TAGPfact:
        return ("fact");
    case TAGPconst:
        return ("const");
    case TAGPflag:
        return ("flag");
    default:
        return ("null");
    }
}

/**
 * Write a systemtemplate to a file in Json-format
 *
 * @param fp file
 * @param st systemtemplate
 */
void write_sys_json(FILE *fp, systemtemplate st) {
    cJSON *json_root, *params, *key, *val;
    syspar_list p;
    parmval pv;
    char *json_out;

    if (st == systemtemplateNIL) {
        return;
    }

    json_root = cJSON_CreateObject();

    if (st->info != tmstringNIL) {
        cJSON_AddItemToObject(json_root, "info", cJSON_CreateString(st->info));
    } else {
        cJSON_AddItemToObject(json_root, "info",
                              cJSON_CreateString("no information available"));
    }

    if (st->model != tmstringNIL) {
        cJSON_AddItemToObject(json_root, "model",
                              cJSON_CreateString(st->model));
    } else {
        cJSON_AddItemToObject(
            json_root, "model",
            cJSON_CreateString("no model information available"));
    }

    cJSON_AddItemToObject(json_root, "parameters",
                          params = cJSON_CreateObject());
    p = st->parm;
    while (p != sysparNIL) {

        cJSON_AddItemToObject(params, p->name, key = cJSON_CreateObject());

        pv = p->val;

        cJSON_AddItemToObject(key, "type",
                              cJSON_CreateString(parmtag2string(pv->tag)));
        cJSON_AddItemToObject(key, "val", val = cJSON_CreateArray());

        switch (pv->tag) {
        case TAGPunkn:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Punkn(pv)->unknval));
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Punkn(pv)->unknlval));
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Punkn(pv)->unknuval));
            break;

        case TAGPmeas:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pmeas(pv)->measval));
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pmeas(pv)->measint));
            break;

        case TAGPcalc:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pcalc(pv)->calcval));
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pcalc(pv)->calcint));
            break;

        case TAGPfact:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pfact(pv)->factval));
            break;

        case TAGPconst:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pconst(pv)->constval));
            break;

        case TAGPflag:
            cJSON_AddItemToArray(val,
                                 cJSON_CreateNumber(to_Pflag(pv)->flagval));
            break;

        default:
            break;
        }

        p = p->next;
    }

    json_out = cJSON_Print(json_root);
    cJSON_Delete(json_root);
    fprintf(fp, "%s\n", json_out);
    free(json_out);

    return;
}
