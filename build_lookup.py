#!/usr/bin/env python3

from sys import argv
import numpy

# Degree of polynomial best representing each column
degs = [3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 2, 2, 3, 1, 1, 1, 1, 1, 1, 1]

def getPolynomials(lookup, degs):
    polynomials = []
    
    nRow, nCol = lookup.shape[0], lookup.shape[1]
    for i in range(nCol):
        pol =  numpy.polyfit(numpy.arange(0, nRow), lookup[:,i], degs[i])
        polynomials.append(pol)
    return polynomials

def buildLookup(polynomials, lookup):
    for i in range(lookup.shape[1]):
        lookup[:,i] = numpy.polyval(polynomials[i], numpy.arange(0, lookup.shape[0]))

def writeLookup(filename, lookup):
    f = open(filename, 'wb')
    ba = bytearray([n for n in lookup.flat])
    f.write(ba)

def writeLookupAsC(filename, lookup):
    f = open(filename, 'w')
    f.write('// AUTO-GENERATED BY build_lookup.py\n')
    f.write('unsigned char lookupNew[357] = {\n')
    for i in range(357//21):
        f.write('   ')
        for j in range(21):
            f.write(' ' + hex(lookup.flat[i*21 + j]) + ',')
        f.write('\n')
    f.write('};\n')

if __name__=='__main__':
    numpy.set_printoptions(threshold=numpy.nan)

    lookup = numpy.loadtxt(argv[1], dtype=int)
    
    # Get rid of the last entry which is "brightness dimmed after inactivity"
    lookup_sane = numpy.delete(lookup, (lookup.shape[0] - 1), axis=0)
    polynomials = getPolynomials(lookup_sane, degs)
    buildLookup(polynomials, lookup_sane)
    lookup[0:lookup.shape[0]-1,:] = lookup_sane
    lookup = lookup.astype(int)
    
    numpy.savetxt('lookupNew.txt', lookup, fmt='%.d')

    writeLookupAsC('lookupNew.h', lookup)