/*
 * ParX - prob.c
 * Standard Probability Distributions
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
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "parx.h"
#include "prob.h"

/* Gamma function for real arguments */

fnum gammaln(fnum x) {
    fnum z, series, denom;
    inum i;
    static fnum c[6] = {76.18009172947146, -86.50532032941677,
        24.01409824083091, -1.231739572450155,
        0.120865097386617e-2, -0.5395239384953e-5};
    
    if (x <= 0.0) return (INF);
    
    z = x + 5.5;
    z -= (x + 0.5) * log(z);
    
    series = 1.000000000190015;
    denom = x;
    for (i = 0; i <= 5; i++)
        series += c[i] / ++denom;
    
    return ( -z + log(2.5066282746310005 * series / x));
}

/* Incomplete Gamma function P, by series representation */

static fnum gamma_sr(fnum x, fnum a) {
    fnum series, delta, denom, gln;
    inum i;
    
    gln = gammaln(a);
    
    delta = series = 1.0 / a;
    denom = a;
    for (i = 1; i <= 100; i++) {
        delta *= x / ++denom;
        series += delta;
        if (fabs(delta) < fabs(series) * DBL_EPSILON)
            return (series * exp(-x + a * log(x) - gln));
    }
    return (0.0);
}

/* Incomplete Gamma function Q, by continued fraction representation */

static fnum gamma_cf(fnum x, fnum a) {
    fnum num, denom, c, d, delta, frac, gln;
    inum i;
    
    gln = gammaln(a);
    
    denom = x + 1.0 - a;
    c = 1.0 / DBL_MIN;
    d = 1.0 / denom;
    frac = d;
    for (i = 1; i <= 100; i++) {
        num = -(double) i * ((double) i - a);
        denom += 2.0;
        d = num * d + denom;
        if (fabs(d) < DBL_MIN) d = DBL_MIN;
        c = denom + num / c;
        if (fabs(c) < DBL_MIN) c = DBL_MIN;
        d = 1.0 / d;
        delta = d * c;
        frac *= delta;
        if (fabs(delta - 1.0) < DBL_EPSILON)
            return (frac * exp(-x + a * log(x) - gln));
    }
    return (1.0);
}

/* Incomplete Gamma function P */

fnum gammap(fnum x, fnum a) {
    if (a <= 0.0) return (1.0);
    if (x <= 0.0) return (0.0);
    if (x < (a + 1.0)) return (gamma_sr(x, a));
    return (1.0 - gamma_cf(x, a));
}

/* Incomplete Gamma function Q = 1 - P */

fnum gammaq(fnum x, fnum a) {
    if (a <= 0.0) return (0.0);
    if (x <= 0.0) return (1.0);
    if (x < (a + 1.0)) return (1.0 - gamma_sr(x, a));
    return (gamma_cf(x, a));
}

/* Chi square probability distribution */

fnum chi2(fnum chi2, inum fr) {
    fnum a, x;
    
    a = 0.5 * (fnum) fr;
    x = 0.5 * chi2;
    return (gammaq(x, a));
}
