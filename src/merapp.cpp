/*
Copyright (c) 2016, UT-Battelle, LLC

MERA++, Version 0.

This file is part of MERA++.
MERA++ is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
MERA++ is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with MERA++. If not, see <http://www.gnu.org/licenses/>.
*/
/** \ingroup MERA */
/*@{*/

/*! \file main.cpp
 *
 *  The MERA main driver
 *
 */

#include <unistd.h>
#include "MeraEnviron.h"
#include <fstream>
#include "MeraToTikz.h"
#include "Version.h"
#include "DimensionSrep.h"

void main1(PsimagLite::String srep,
           SizeType tauMax,
           SizeType d)
{
	Mera::DimensionSrep dimSrep(srep,d);
	PsimagLite::String dsrep = dimSrep();
	dsrep += "h0(" + ttos(d) + "," + ttos(d) + "|" + ttos(d) + "," + ttos(d) + ")";
	dsrep += "i0(" + ttos(d) + "|" + ttos(d) + ")";
	std::cout<<"DimensionSrep="<<dsrep<<"\n";
	//std::cout<<"DimensionSrep=u0(2,2|4)u1(2,2|2,2)w0(4,2|8)w1(2,2)h0(2,2|2,2)r(8,2)i0(2|2)\n";

	Mera::ParametersForSolver params(tauMax);
	Mera::MeraEnviron environ(srep,params);

	environ.computeEnvirons();
	std::cerr<<environ;
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		std::cerr<<"USAGE: "<<argv[0]<<" tauMax\n";
		return 1;
	}

	PsimagLite::String str("");
	SizeType d = 2;

	if (argc == 2)
		str = "u0(f0,f1|s0)u1(f2,f3|s1,s2)w0(s0,s1|s3)w1(s2|s4)r(s3,s4)\n";

	if (argc > 2) {
		char c = '\0';
		while (std::cin>>c) {
			str += c;
		}
	}

	std::cout<<"#"<<argv[0]<<" version "<<MERA_VERSION<<"\n";
	SizeType tauMax = atoi(argv[1]);
	std::cout<<"TauMax="<<tauMax<<"\n";
	main1(str,tauMax,d);
}
