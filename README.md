# ParXCL

The algorithms in ParX were developed as part of my Ph.D. thesis in April of 1992, as was their first implementation.

ParX started life as a HP-UX command line tool. 
A Yacc and Lex parser provided the user interface. 
The C code base was build on top of a home-spun OO-framework TM that was developed by my friend and then colleague Kees van Reeuwijk.

Because of the limited computer resources at that time, 
the code is highly optimized for memory re-use and very “smart” pointer arithmetic; 
trying to fit every inner loop in the miniscule processor caches of the day.

The Linear Algebra heavy lifting was delegated to the commercial NAG™ library, 
which in later incarnations was transplanted by LAPACK and BLAS. 
Both libraries are written in FORTRAN, 
so the vector and matrix data structures in ParX follow the FORTRAN memory-layout conventions.

The Model Compiler was still in the early planning stages, 
so initially all models equations and their first-order derivatives had to be hand- and hard-coded in C. 
The Model Compiler was finally connected to the parser via POSIX pipes.
Now it is integrated in ParXCL.

ParXCL is still offered (as is) for embedded applications on the MacOS, Linux and Windows (via Cygwin) platforms. 

