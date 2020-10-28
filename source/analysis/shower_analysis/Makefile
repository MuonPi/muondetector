# Makefile for the ROOT test programs.
# This Makefile shows nicely how to compile and link applications
# using the ROOT libraries on all supported platforms.
#
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000
# Amended by: Lukas Nies, 01/10/2019

ARCH          = linux

# CXX           = 

CXX           = g++
ObjSuf        = o
SrcSuf        = cxx
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o 

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)


ifeq ($(ARCH),linux)
# Linux with egcs, gcc 2.9x, gcc 3.x (>= RedHat 5.2)
CXX           = g++
CXXFLAGS      = -Wall -g
LD            = g++
LDFLAGS       = -g
SOFLAGS       = -shared
endif

# LDFlags was -O

CXXFLAGS     += $(ROOTCFLAGS)
LIBS          = $(ROOTLIBS) $(SYSLIBS)-lMathMore -lSpectrum -lRooStats -lRooFit -lRooFitCore -lHtml -lMinuit 
GLIBS         = $(ROOTGLIBS) $(SYSLIBS)-lMathMore -lSpectrum -lRooStats -lRooFit -lRooFitCore -lHtml -lMinuit 

#------------------------------------------------------------------------------

SHOWER_ANALYSISO   = shower_analysis.o 
SHOWER_ANALYSISS   = shower_analysis.$(SrcSuf) 
SHOWER_ANALYSIS    = shower_analysis$(ExeSuf)

SHOWER_SIMULATEO   = shower_simulate.o 
SHOWER_SIMULATES   = shower_simulate.$(SrcSuf) 
SHOWER_SIMULATE    = shower_simulate$(ExeSuf)


#------------------------------------------------------------------------------

.SUFFIXES: .$(SrcSuf) .$(ObjSuf) .$(DllSuf)


all:            $(PROGRAMS)

$(SHOWER_ANALYSIS):     $(SHOWER_ANALYSISO)
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		@echo "$@ done"

$(SHOWER_SIMULATE):     $(SHOWER_SIMULATEO)
		$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@
		@echo "$@ done"


clean:
		@rm -f $(OBJS) core

distclean:      clean
		@rm -f $(PROGRAMS) $(EVENTSO) $(EVENTLIB) *Dict.* *.def *.exp \
		   *.root *.ps *.so .def so_locations
		@rm -rf cxx_repository

.SUFFIXES: .$(SrcSuf)

###


.$(SrcSuf).$(ObjSuf):
	$(CXX) $(CXXFLAGS) -c $<

