////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	softmpy.cpp
//
// Project:	A General Purpose Pipelined FFT Implementation
//
// Purpose:	If the chip doesn't have any hardware multiplies, you'll need
// 		a soft-multiply implementation.  This provides that
// 	implementation.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2018, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory, run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
#define _CRT_SECURE_NO_WARNINGS   //  ms vs 2012 doesn't like fopen
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER //  added for ms vs compatibility

#include <io.h>
#include <direct.h>
#define _USE_MATH_DEFINES

#endif

#include <string.h>
#include <string>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#include "defaults.h"
#include "legal.h"
#include "softmpy.h"

void	build_multiply(const char *fname) {
	FILE	*fp = fopen(fname, "w");
	if (NULL == fp) {
		fprintf(stderr, "Could not open \'%s\' for writing\n", fname);
		perror("O/S Err was:");
		return;
	}

	fprintf(fp,
SLASHLINE
"//\n"
"// Filename:\tshiftaddmpy.v\n"
"//\n"
"// Project:\t%s\n"
"//\n"
"// Purpose:\tA portable shift and add multiply.\n"
"//\n"
"//	While both Xilinx and Altera will offer single clock multiplies, this\n"
"//	simple approach will multiply two numbers on any architecture.  The\n"
"//	result maintains the full width of the multiply, there are no extra\n"
"//	stuff bits, no rounding, no shifted bits, etc.\n"
"//\n"
"//	Further, for those applications that can support it, this multiply\n"
"//	is pipelined and will produce one answer per clock.\n"
"//\n"
"//	For minimal processing delay, make the first parameter the one with\n"
"//	the least bits, so that AWIDTH <= BWIDTH.\n"
"//\n"
"//	The processing delay in this multiply is (AWIDTH+1) cycles.  That is,\n"
"//	if the data is present on the input at clock t=0, the result will be\n"
"//	present on the output at time t=AWIDTH+1;\n"
"//\n"
"//\n%s"
"//\n", prjname, creator);

	fprintf(fp, "%s", cpyleft);
	fprintf(fp, "//\n//\n`default_nettype\tnone\n//\n");
	fprintf(fp,
"module	shiftaddmpy(i_clk, i_ce, i_a, i_b, o_r);\n"
	"\tparameter\tAWIDTH=%d,BWIDTH=", TST_SHIFTADDMPY_AW);
#ifdef	TST_SHIFTADDMPY_BW
	fprintf(fp, "%d;\n", TST_SHIFTADDMPY_BW);
#else
	fprintf(fp, "AWIDTH;\n");
#endif
	fprintf(fp,
	"\tinput\t\t\t\t\ti_clk, i_ce;\n"
	"\tinput\t\t[(AWIDTH-1):0]\t\ti_a;\n"
	"\tinput\t\t[(BWIDTH-1):0]\t\ti_b;\n"
	"\toutput\treg\t[(AWIDTH+BWIDTH-1):0]\to_r;\n"
"\n"
	"\treg\t[(AWIDTH-1):0]\tu_a;\n"
	"\treg\t[(BWIDTH-1):0]\tu_b;\n"
	"\treg\t\t\tsgn;\n"
"\n"
	"\treg\t[(AWIDTH-2):0]\t\tr_a[0:(AWIDTH-1)];\n"
	"\treg\t[(AWIDTH+BWIDTH-2):0]\tr_b[0:(AWIDTH-1)];\n"
	"\treg\t\t\t\tr_s[0:(AWIDTH-1)];\n"
	"\treg\t[(AWIDTH+BWIDTH-1):0]\tacc[0:(AWIDTH-1)];\n"
	"\tgenvar k;\n"
"\n"
	"\t// If we were forced to stay within two\'s complement arithmetic,\n"
	"\t// taking the absolute value here would require an additional bit.\n"
	"\t// However, because our results are now unsigned, we can stay\n"
	"\t// within the number of bits given (for now).\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
		"\t\tbegin\n"
			"\t\t\tu_a <= (i_a[AWIDTH-1])?(-i_a):(i_a);\n"
			"\t\t\tu_b <= (i_b[BWIDTH-1])?(-i_b):(i_b);\n"
			"\t\t\tsgn <= i_a[AWIDTH-1] ^ i_b[BWIDTH-1];\n"
		"\t\tend\n"
"\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
		"\t\tbegin\n"
			"\t\t\tacc[0] <= (u_a[0]) ? { {(AWIDTH){1\'b0}}, u_b }\n"
			"\t\t\t\t\t: {(AWIDTH+BWIDTH){1\'b0}};\n"
			"\t\t\tr_a[0] <= { u_a[(AWIDTH-1):1] };\n"
			"\t\t\tr_b[0] <= { {(AWIDTH-1){1\'b0}}, u_b };\n"
			"\t\t\tr_s[0] <= sgn; // The final sign, needs to be preserved\n"
		"\t\tend\n"
"\n"
	"\tgenerate\n"
	"\tfor(k=0; k<AWIDTH-1; k=k+1)\n"
	"\tbegin : genstages\n"
		"\t\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
		"\t\tbegin\n"
			"\t\t\tacc[k+1] <= acc[k] + ((r_a[k][0]) ? {r_b[k],1\'b0}:0);\n"
			"\t\t\tr_a[k+1] <= { 1\'b0, r_a[k][(AWIDTH-2):1] };\n"
			"\t\t\tr_b[k+1] <= { r_b[k][(AWIDTH+BWIDTH-3):0], 1\'b0};\n"
			"\t\t\tr_s[k+1] <= r_s[k];\n"
		"\t\tend\n"
	"\tend\n"
	"\tendgenerate\n"
"\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
			"\t\t\to_r <= (r_s[AWIDTH-1]) ? (-acc[AWIDTH-1]) : acc[AWIDTH-1];\n"
"\n"
"endmodule\n");

	fclose(fp);
}

void	build_bimpy(const char *fname) {
	FILE	*fp = fopen(fname, "w");
	if (NULL == fp) {
		fprintf(stderr, "Could not open \'%s\' for writing\n", fname);
		perror("O/S Err was:");
		return;
	}

	fprintf(fp,
SLASHLINE
"//\n"
"// Filename:\t%s\n"
"//\n"
"// Project:\t%s\n"
"//\n"
"// Purpose:\tA simple 2-bit multiply based upon the fact that LUT's allow\n"
"//		6-bits of input.  In other words, I could build a 3-bit\n"
"//	multiply from 6 LUTs (5 actually, since the first could have two\n"
"//	outputs).  This would allow multiplication of three bit digits, save\n"
"//	only for the fact that you would need two bits of carry.  The bimpy\n"
"//	approach throttles back a bit and does a 2x2 bit multiply in a LUT,\n"
"//	guaranteeing that it will never carry more than one bit.  While this\n"
"//	multiply is hardware independent (and can still run under Verilator\n"
"//	therefore), it is really motivated by trying to optimize for a\n"
"//	specific piece of hardware (Xilinx-7 series ...) that has at least\n"
"//	4-input LUT's with carry chains.\n"
"//\n"
"//\n"
"//\n%s"
"//\n", fname, prjname, creator);

	fprintf(fp, "%s", cpyleft);
	fprintf(fp, "//\n//\n`default_nettype\tnone\n//\n");
	fprintf(fp,
"module	bimpy(i_clk, i_ce, i_a, i_b, o_r);\n"
"\tparameter\tBW=18, // Number of bits in i_b\n"
"\t\t\tLUTB=2; // Number of bits in i_a for our LUT multiply\n"
"\tinput\t\t\t\ti_clk, i_ce;\n"
"\tinput\t\t[(LUTB-1):0]\ti_a;\n"
"\tinput\t\t[(BW-1):0]\ti_b;\n"
"\toutput\treg\t[(BW+LUTB-1):0]	o_r;\n"
"\n"
"\twire	[(BW+LUTB-2):0]	w_r;\n"
"\twire	[(BW+LUTB-3):1]	c;\n"
"\n"
"\tassign\tw_r =  { ((i_a[1])?i_b:{(BW){1\'b0}}), 1\'b0 }\n"
"\t\t\t\t^ { 1\'b0, ((i_a[0])?i_b:{(BW){1\'b0}}) };\n"
"\tassign\tc = { ((i_a[1])?i_b[(BW-2):0]:{(BW-1){1\'b0}}) }\n"
"\t\t\t& ((i_a[0])?i_b[(BW-1):1]:{(BW-1){1\'b0}});\n"
"\n"
"\talways @(posedge i_clk)\n"
"\t\tif (i_ce)\n"
"\t\t\to_r <= w_r + { c, 2'b0 };\n"
"\n"
"endmodule\n");

	fclose(fp);
}

void	build_longbimpy(const char *fname) {
	FILE	*fp = fopen(fname, "w");
	if (NULL == fp) {
		fprintf(stderr, "Could not open \'%s\' for writing\n", fname);
		perror("O/S Err was:");
		return;
	}

	fprintf(fp,
SLASHLINE
"//\n"
"// Filename: 	%s\n"
"//\n"
"// Project:	%s\n"
"//\n"
"// Purpose:	A portable shift and add multiply, built with the knowledge\n"
"//	of the existence of a six bit LUT and carry chain.  That knowledge\n"
"//	allows us to multiply two bits from one value at a time against all\n"
"//	of the bits of the other value.  This sub multiply is called the\n"
"//	bimpy.\n"
"//\n"
"//	For minimal processing delay, make the first parameter the one with\n"
"//	the least bits, so that AWIDTH <= BWIDTH.\n"
"//\n"
"//\n"
"//\n%s"
"//\n", fname, prjname, creator);

	fprintf(fp, "%s", cpyleft);
	fprintf(fp, "//\n//\n`default_nettype\tnone\n//\n");
	fprintf(fp,
"module	longbimpy(i_clk, i_ce, i_a_unsorted, i_b_unsorted, o_r);\n"
	"\tparameter	IAW=%d,	// The width of i_a, min width is 5\n"
			"\t\t\tIBW=", TST_LONGBIMPY_AW);
#ifdef	TST_LONGBIMPY_BW
	fprintf(fp, "%d", TST_LONGBIMPY_BW);
#else
	fprintf(fp, "IAW");
#endif

	fprintf(fp, ",	// The width of i_b, can be anything\n"
			"\t\t\t// The following three parameters should not be changed\n"
			"\t\t\t// by any implementation, but are based upon hardware\n"
			"\t\t\t// and the above values:\n"
			"\t\t\tOW=IAW+IBW;	// The output width\n");
	fprintf(fp,
	"\tlocalparam	AW = (IAW<IBW) ? IAW : IBW,\n"
			"\t\t\tBW = (IAW<IBW) ? IBW : IAW,\n"
			"\t\t\tIW=(AW+1)&(-2),	// Internal width of A\n"
			"\t\t\tLUTB=2,	// How many bits we can multiply by at once\n"
			"\t\t\tTLEN=(AW+(LUTB-1))/LUTB; // Nmbr of rows in our tableau\n"
	"\tinput\t\t\t\ti_clk, i_ce;\n"
	"\tinput\t\t[(IAW-1):0]\ti_a_unsorted;\n"
	"\tinput\t\t[(IBW-1):0]\ti_b_unsorted;\n"
	"\toutput\treg\t[(AW+BW-1):0]\to_r;\n"
"\n"
	"\t//\n"
	"\t// Swap parameter order, so that AW <= BW -- for performance\n"
	"\t// reasons\n"
	"\twire	[AW-1:0]	i_a;\n"
	"\twire	[BW-1:0]	i_b;\n"
	"\tgenerate if (IAW <= IBW)\n"
	"\tbegin : NO_PARAM_CHANGE\n"
	"\t\tassign i_a = i_a_unsorted;\n"
	"\t\tassign i_b = i_b_unsorted;\n"
	"\tend else begin : SWAP_PARAMETERS\n"
	"\t\tassign i_a = i_b_unsorted;\n"
	"\t\tassign i_b = i_a_unsorted;\n"
	"\tend endgenerate\n"
"\n"
	"\treg\t[(IW-1):0]\tu_a;\n"
	"\treg\t[(BW-1):0]\tu_b;\n"
	"\treg\t\t\tsgn;\n"
"\n"
	"\treg\t[(IW-1-2*(LUTB)):0]\tr_a[0:(TLEN-3)];\n"
	"\treg\t[(BW-1):0]\t\tr_b[0:(TLEN-3)];\n"
	"\treg\t[(TLEN-1):0]\t\tr_s;\n"
	"\treg\t[(IW+BW-1):0]\t\tacc[0:(TLEN-2)];\n"
	"\tgenvar k;\n"
"\n"
	"\t// First step:\n"
	"\t// Switch to unsigned arithmetic for our multiply, keeping track\n"
	"\t// of the along the way.  We'll then add the sign again later at\n"
	"\t// the end.\n"
	"\t//\n"
	"\t// If we were forced to stay within two's complement arithmetic,\n"
	"\t// taking the absolute value here would require an additional bit.\n"
	"\t// However, because our results are now unsigned, we can stay\n"
	"\t// within the number of bits given (for now).\n"
	"\tgenerate if (IW > AW)\n"
	"\tbegin\n"
		"\t\talways @(posedge i_clk)\n"
			"\t\t\tif (i_ce)\n"
			"\t\t\t\tu_a <= { 1\'b0, (i_a[AW-1])?(-i_a):(i_a) };\n"
	"\tend else begin\n"
		"\t\talways @(posedge i_clk)\n"
			"\t\t\tif (i_ce)\n"
			"\t\t\t\tu_a <= (i_a[AW-1])?(-i_a):(i_a);\n"
	"\tend endgenerate\n"
"\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
		"\t\tbegin\n"
			"\t\t\tu_b <= (i_b[BW-1])?(-i_b):(i_b);\n"
			"\t\t\tsgn <= i_a[AW-1] ^ i_b[BW-1];\n"
		"\t\tend\n"
"\n"
	"\twire	[(BW+LUTB-1):0]	pr_a, pr_b;\n"
"\n"
	"\t//\n"
	"\t// Second step: First two 2xN products.\n"
	"\t//\n"
	"\t// Since we have no tableau of additions (yet), we can do both\n"
	"\t// of the first two rows at the same time and add them together.\n"
	"\t// For the next round, we'll then have a previous sum to accumulate\n"
	"\t// with new and subsequent product, and so only do one product at\n"
	"\t// a time can follow this--but the first clock can do two at a time.\n"
	"\tbimpy\t#(BW) lmpy_0(i_clk,i_ce,u_a[(  LUTB-1):   0], u_b, pr_a);\n"
	"\tbimpy\t#(BW) lmpy_1(i_clk,i_ce,u_a[(2*LUTB-1):LUTB], u_b, pr_b);\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce) r_a[0] <= u_a[(IW-1):(2*LUTB)];\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce) r_b[0] <= u_b;\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce) r_s <= { r_s[(TLEN-2):0], sgn };\n"
	"\talways @(posedge i_clk) // One clk after p[0],p[1] become valid\n"
		"\t\tif (i_ce) acc[0] <= { {(IW-LUTB){1\'b0}}, pr_a}\n"
			"\t\t\t  +{ {(IW-(2*LUTB)){1\'b0}}, pr_b, {(LUTB){1\'b0}} };\n"
"\n"
	"\tgenerate // Keep track of intermediate values, before multiplying them\n"
	"\tif (TLEN > 3) for(k=0; k<TLEN-3; k=k+1)\n"
	"\tbegin : gencopies\n"
		"\t\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
		"\t\tbegin\n"
			"\t\t\tr_a[k+1] <= { {(LUTB){1\'b0}},\n"
				"\t\t\t\tr_a[k][(IW-1-(2*LUTB)):LUTB] };\n"
			"\t\t\tr_b[k+1] <= r_b[k];\n"
			"\t\tend\n"
	"\tend endgenerate\n"
"\n"
	"\tgenerate // The actual multiply and accumulate stage\n"
	"\tif (TLEN > 2) for(k=0; k<TLEN-2; k=k+1)\n"
	"\tbegin : genstages\n"
		"\t\t// First, the multiply: 2-bits times BW bits\n"
		"\t\twire\t[(BW+LUTB-1):0] genp;\n"
		"\t\tbimpy #(BW) genmpy(i_clk,i_ce,r_a[k][(LUTB-1):0],r_b[k], genp);\n"
"\n"
		"\t\t// Then the accumulate step -- on the next clock\n"
		"\t\talways @(posedge i_clk)\n"
			"\t\t\tif (i_ce)\n"
				"\t\t\t\tacc[k+1] <= acc[k] + {{(IW-LUTB*(k+3)){1\'b0}},\n"
					"\t\t\t\t\tgenp, {(LUTB*(k+2)){1\'b0}} };\n"
	"\tend endgenerate\n"
"\n"
	"\twire	[(IW+BW-1):0]	w_r;\n"
	"\tassign\tw_r = (r_s[TLEN-1]) ? (-acc[TLEN-2]) : acc[TLEN-2];\n"
	"\talways @(posedge i_clk)\n"
		"\t\tif (i_ce)\n"
			"\t\t\to_r <= w_r[(AW+BW-1):0];\n"
"\n"
	"\tgenerate if (IW > AW)\n"
	"\tbegin : VUNUSED\n"
	"\t\t// verilator lint_off UNUSED\n"
	"\t\twire\t[(IW-AW)-1:0]\tunused;\n"
	"\t\tassign\tunused = w_r[(IW+BW-1):(AW+BW)];\n"
	"\t\t// verilator lint_on UNUSED\n"
	"\tend endgenerate\n"
"\n"
"endmodule\n");

	fclose(fp);
}

