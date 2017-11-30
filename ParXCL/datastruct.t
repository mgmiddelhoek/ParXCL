.. ParX - datastruct.t
.. TM representation of parx data structures
..
.. Copyright (c) 2009 M.G.Middelhoek <martin@middelhoek.com>
..
.. This program is free software: you can redistribute it and/or modify
.. it under the terms of the GNU General Public License as published by
.. the Free Software Foundation, either version 3 of the License, or
.. (at your option) any later version.
..
.. This program is distributed in the hope that it will be useful,
.. but WITHOUT ANY WARRANTY; without even the implied warranty of
.. MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
.. GNU General Public License for more details.
..
.. You should have received a copy of the GNU General Public License
.. along with this program.  If not, see <https://www.gnu.org/licenses/>.
..
.. Tm specification file
.. Using C list-implementation
..
.set basename parx
..
.. functions on primitive types are defined in primtype.t 
.set notwantdefs fnum print_fnum fscan_fnum 
.append notwantdefs fnum_list new_fnum_list fre_fnum_list rfre_fnum_list 
.append notwantdefs append_fnum_list insert_fnum_list rdup_fnum_list 
.append notwantdefs print_fnum_list fscan_fnum_list
..
.append notwantdefs inum print_inum fscan_inum 
.append notwantdefs inum_list new_inum_list fre_inum_list rfre_inum_list 
.append notwantdefs append_inum_list insert_inum_list rdup_inum_list 
.append notwantdefs print_inum_list fscan_inum_list
..
.append notwantdefs boolean print_boolean fscan_boolean 
.append notwantdefs boolean_list new_boolean_list fre_boolean_list rfre_boolean_list 
.append notwantdefs append_boolean_list insert_boolean_list rdup_boolean_list 
.append notwantdefs print_boolean_list fscan_boolean_list
..
.append notwantdefs stateflag print_stateflag fscan_stateflag 
.append notwantdefs stateflag_list new_stateflag_list fre_stateflag_list rfre_stateflag_list 
.append notwantdefs append_stateflag_list insert_stateflag_list rdup_stateflag_list 
.append notwantdefs print_stateflag_list fscan_stateflag_list
..
.append notwantdefs vector print_vector 
.append notwantdefs matrix print_matrix 
.append notwantdefs inumvector print_inumvector 
.append notwantdefs boolvector print_boolvector 
.append notwantdefs statevector print_statevector
..
..
.set wantdefs stat_$(basename)
..
.foreach t ${typelist}
.append wantdefs new_$t
.endforeach
..
.append wantdefs new_dbnode
.append wantdefs new_dbnode_list
.append wantdefs rfre_dbnode
.append	wantdefs rfre_dbnode_list
.append wantdefs append_dbnode_list
.append wantdefs insert_dbnode_list
.append wantdefs rdup_dbnode
.append wantdefs print_dbnode
.append wantdefs print_dbnode_list
.append wantdefs fscan_dbnode
.append wantdefs fscan_dbnode_list
..
.append wantdefs new_stimtemplate_list rfre_stimtemplate rfre_stimtemplate_list 
.append wantdefs append_stimtemplate_list rdup_stimtemplate_list
.append wantdefs new_meastemplate_list rfre_meastemplate rfre_meastemplate_list 
.append wantdefs append_meastemplate_list rdup_meastemplate_list
..
.append wantdefs new_parxsymbol_list
.append wantdefs append_parxsymbol_list
.append wantdefs concat_parxsymbol_list
..
.append wantdefs new_parsenode_list
.append wantdefs insert_parsenode_list
.append wantdefs rfre_parsenode_list
..
..
.append wantdefs rfre_modeltemplate fscan_modeltemplate print_modeltemplate
.append wantdefs rdup_modeltemplate
.append wantdefs print_pspec_list
..
..
.append wantdefs rfre_systemtemplate fscan_systemtemplate print_systemtemplate
.append wantdefs rdup_systemtemplate
.append wantdefs fre_parmval
..
.append wantdefs append_syspar_list
.append wantdefs new_syspar_list
.. 
..
.append wantdefs rfre_datatemplate fscan_datatemplate print_datatemplate
.append wantdefs rdup_datatemplate
..
.append wantdefs new_colhead_list rdup_colhead
.append wantdefs rdup_colhead_list
.append wantdefs append_colhead_list
..
.append wantdefs new_datarow_list
.append wantdefs append_datarow_list
.append wantdefs rfre_datarow
.append wantdefs rdup_datarow
.append wantdefs print_datarow
..
..
.append wantdefs rfre_numblock
.append wantdefs print_numblock
..
.append wantdefs rfre_xgroup
.append wantdefs append_xgroup_list
.append wantdefs new_xgroup_list
..set notwantdefs aset_list cset_list fset_list pset_list xset_list
..
..
.append wantdefs rfre_modreq fre_modreq
.append wantdefs rfre_modres print_modres
.append wantdefs rfre_moddat fre_moddat print_moddat
..
.append wantdefs append_pset_list
.append wantdefs fre_pset
.append wantdefs print_pset
.append wantdefs print_pset_list
.append wantdefs rdup_pset
.append wantdefs rfre_pset
.append wantdefs rfre_pset_list
.append wantdefs new_pset_list
..
.append wantdefs append_cset_list
.append wantdefs fre_cset
.append wantdefs print_cset
.append wantdefs print_cset_list
.append wantdefs rdup_cset
.append wantdefs rfre_cset
.append wantdefs rfre_cset_list
.append wantdefs new_cset_list
..
.append wantdefs append_fset_list
.append wantdefs fre_fset
.append wantdefs print_fset
.append wantdefs print_fset_list
.append wantdefs rdup_fset
.append wantdefs rfre_fset
.append wantdefs rfre_fset_list
.append wantdefs new_fset_list
..
.append wantdefs append_xset_list
.append wantdefs insert_xset_list
.append wantdefs fre_xset
.append wantdefs new_xset_list
.append wantdefs print_xset
.append wantdefs print_xset_list
.append wantdefs rdup_xset
.append wantdefs rfre_xset
.append wantdefs fre_xset_list
.append wantdefs rfre_xset_list
..
.append wantdefs append_aset_list
.append wantdefs fre_aset
.append wantdefs print_aset
.append wantdefs print_aset_list
.append wantdefs rdup_aset
.append wantdefs rfre_aset
.append wantdefs rfre_aset_list
.append wantdefs new_aset_list
..
