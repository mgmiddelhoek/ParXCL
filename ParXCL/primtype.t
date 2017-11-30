.. ParX - primtype.t
.. TM data structure definition for primitive types
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
.. Using C array-implementation
..
.set basename primtype
..
.set wantdefs stat_$(basename)
..
.append wantdefs fnum_list inum_list stateflag_list boolean_list
..
.append wantdefs new_fnum_list fre_fnum_list append_fnum_list 
.append wantdefs rdup_fnum_list
.append wantdefs fscan_fnum_list print_fnum_list
..
.append wantdefs new_inum_list fre_inum_list append_inum_list
.append wantdefs rdup_inum_list fscan_inum_list print_inum_list
..
.append wantdefs new_boolean_list fre_boolean_list append_boolean_list
.append wantdefs new_stateflag_list fre_stateflag_list append_stateflag_list
..
.append wantdefs room_fnum_list room_inum_list 
.append wantdefs room_boolean_list room_stateflag_list
..
.. we don't want code for vector and matrix types only types
.append wantdefs statevector boolvector inumvector vector matrix
